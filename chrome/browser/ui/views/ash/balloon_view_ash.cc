// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/ash/balloon_view_ash.h"

#include "ash/shell.h"
#include "ash/system/web_notification/web_notification_tray.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chrome/browser/favicon/favicon_util.h"
#include "chrome/browser/notifications/balloon_collection.h"
#include "chrome/browser/notifications/notification.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_constants.h"
#include "webkit/glue/image_resource_fetcher.h"

namespace {

typedef base::Callback<void(const gfx::ImageSkia&)> SetImageCallback;

const int kPrimaryIconImageSize = 64;
const int kSecondaryIconImageSize = 15;

// static
message_center::MessageCenter* GetMessageCenter() {
  return ash::Shell::GetInstance()->GetWebNotificationTray()->message_center();
}

}  // namespace

// TODO(dharcourt): Delay showing the notification until all images are
// downloaded, and return an error to the notification creator/API caller
// instead of showing a partial notification if any image download fails.
class BalloonViewAsh::ImageDownload
    : public base::SupportsWeakPtr<ImageDownload> {
 public:
  // Note that the setter callback passed in will not be called if the image
  // download fails for any reason.
  ImageDownload(const Notification& notification,
                const GURL& url,
                int size,
                const SetImageCallback& callback);
  virtual ~ImageDownload();

 private:
  // FaviconHelper callback.
  virtual void Downloaded(int download_id,
                          const GURL& image_url,
                          bool errored,
                          int requested_size,
                          const std::vector<SkBitmap>& bitmaps);

  const GURL& url_;
  int size_;
  SetImageCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(ImageDownload);
};

BalloonViewAsh::ImageDownload::ImageDownload(const Notification& notification,
                                             const GURL& url,
                                             int size,
                                             const SetImageCallback& callback)
    : url_(url),
      size_(size),
      callback_(callback) {
  content::RenderViewHost* host = notification.GetRenderViewHost();
  if (!host) {
    LOG(WARNING) << "Notification needs an image but has no RenderViewHost";
    return;
  }

  content::WebContents* contents =
      content::WebContents::FromRenderViewHost(host);
  if (!contents) {
    LOG(WARNING) << "Notification needs an image but has no WebContents";
    return;
  }

  contents->DownloadFavicon(url_, size_, base::Bind(&ImageDownload::Downloaded,
                                                    AsWeakPtr()));
}


BalloonViewAsh::ImageDownload::~ImageDownload() {
}

void BalloonViewAsh::ImageDownload::Downloaded(
    int download_id,
    const GURL& image_url,
    bool errored,
    int requested_size,
    const std::vector<SkBitmap>& bitmaps) {
  if (bitmaps.empty())
    return;
  gfx::ImageSkia image(bitmaps[0]);
  callback_.Run(image);
}

BalloonViewAsh::BalloonViewAsh(BalloonCollection* collection)
    : collection_(collection),
      balloon_(NULL) {
}

BalloonViewAsh::~BalloonViewAsh() {
}

// BalloonView interface.
void BalloonViewAsh::Show(Balloon* balloon) {
  balloon_ = balloon;
  const Notification& notification = balloon_->notification();
  notification_id_ = notification.notification_id();
  GetMessageCenter()->AddNotification(notification.type(),
                                      notification_id_,
                                      notification.title(),
                                      notification.body(),
                                      notification.display_source(),
                                      balloon->GetExtensionId(),
                                      notification.optional_fields());
  DownloadImages(notification);
}

void BalloonViewAsh::Update() {
  std::string previous_notification_id = notification_id_;
  const Notification& notification = balloon_->notification();
  notification_id_ = notification.notification_id();
  GetMessageCenter()->UpdateNotification(previous_notification_id,
                                         notification_id_,
                                         notification.title(),
                                         notification.body(),
                                         notification.optional_fields());
  DownloadImages(notification);
}

void BalloonViewAsh::RepositionToBalloon() {
}

void BalloonViewAsh::Close(bool by_user) {
  Notification notification(balloon_->notification());
  collection_->OnBalloonClosed(balloon_);  // Deletes balloon.
  notification.Close(by_user);
  GetMessageCenter()->RemoveNotification(notification.notification_id());
}

gfx::Size BalloonViewAsh::GetSize() const {
  return gfx::Size();
}

BalloonHost* BalloonViewAsh::GetHost() const {
  return NULL;
}

void BalloonViewAsh::SetNotificationIcon(const std::string& id,
                                         const gfx::ImageSkia& image) {
  GetMessageCenter()->SetNotificationPrimaryIcon(id, image);
}

void BalloonViewAsh::DownloadImages(const Notification& notification) {
  // Cancel any previous downloads.
  downloads_.clear();

  // Set the notification's primary icon, or start a download for it.
  if (!notification.icon().isNull()) {
    SetNotificationIcon(notification_id_, notification.icon());
  } else if (!notification.icon_url().is_empty()) {
      downloads_.push_back(linked_ptr<ImageDownload>(new ImageDownload(
          notification, notification.icon_url(),
          message_center::kNotificationIconWidth,
          base::Bind(&BalloonViewAsh::SetNotificationIcon,
                     base::Unretained(this), notification.notification_id()))));
  }
}
