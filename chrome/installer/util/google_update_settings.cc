// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/google_update_settings.h"

#include "base/registry.h"
#include "base/string_util.h"
#include "base/time.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/google_update_constants.h"

namespace {

bool ReadGoogleUpdateStrKey(const wchar_t* const name, std::wstring* value) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ);
  if (!key.ReadValue(name, value)) {
    RegKey hklm_key(HKEY_LOCAL_MACHINE, reg_path.c_str(), KEY_READ);
    return hklm_key.ReadValue(name, value);
  }
  return true;
}

bool WriteGoogleUpdateStrKey(const wchar_t* const name,
                             const std::wstring& value) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ | KEY_WRITE);
  return key.WriteValue(name, value.c_str());
}

bool ClearGoogleUpdateStrKey(const wchar_t* const name) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ | KEY_WRITE);
  std::wstring value;
  if (!key.ReadValue(name, &value))
    return false;
  return key.WriteValue(name, L"");
}

bool RemoveGoogleUpdateStrKey(const wchar_t* const name) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ | KEY_WRITE);
  if (!key.ValueExists(name))
    return true;
  return key.DeleteValue(name);
}

}  // namespace.

bool GoogleUpdateSettings::GetCollectStatsConsent() {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ);
  DWORD value;
  if (!key.ReadValueDW(google_update::kRegUsageStatsField, &value)) {
    RegKey hklm_key(HKEY_LOCAL_MACHINE, reg_path.c_str(), KEY_READ);
    if (!hklm_key.ReadValueDW(google_update::kRegUsageStatsField, &value))
      return false;
  }
  return (1 == value);
}

bool GoogleUpdateSettings::SetCollectStatsConsent(bool consented) {
  DWORD value = consented? 1 : 0;
  // Writing to HKLM is only a best effort deal.
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateMediumKey();
  RegKey key_hklm(HKEY_LOCAL_MACHINE, reg_path.c_str(), KEY_READ | KEY_WRITE);
  key_hklm.WriteValue(google_update::kRegUsageStatsField, value);
  // Writing to HKCU is used both by chrome and by the crash reporter.
  reg_path = dist->GetStateKey();
  RegKey key_hkcu(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ | KEY_WRITE);
  return key_hkcu.WriteValue(google_update::kRegUsageStatsField, value);
}

bool GoogleUpdateSettings::GetMetricsId(std::wstring* metrics_id) {
  return ReadGoogleUpdateStrKey(google_update::kRegMetricsId, metrics_id);
}

bool GoogleUpdateSettings::SetMetricsId(const std::wstring& metrics_id) {
  return WriteGoogleUpdateStrKey(google_update::kRegMetricsId, metrics_id);
}

bool GoogleUpdateSettings::SetEULAConsent(bool consented) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  std::wstring reg_path = dist->GetStateMediumKey();
  RegKey key(HKEY_LOCAL_MACHINE, reg_path.c_str(), KEY_READ | KEY_SET_VALUE);
  return key.WriteValue(google_update::kRegEULAAceptedField, consented? 1 : 0);
}

int GoogleUpdateSettings::GetLastRunTime() {
 std::wstring time_s;
 if (!ReadGoogleUpdateStrKey(google_update::kRegLastRunTimeField, &time_s))
   return -1;
 int64 time_i;
 if (!StringToInt64(time_s, &time_i))
   return -1;
 base::TimeDelta td =
    base::Time::NowFromSystemTime() - base::Time::FromInternalValue(time_i);
 return td.InDays();
}

bool GoogleUpdateSettings::SetLastRunTime() {
  int64 time = base::Time::NowFromSystemTime().ToInternalValue();
  return WriteGoogleUpdateStrKey(google_update::kRegLastRunTimeField,
                                 Int64ToWString(time));
}

bool GoogleUpdateSettings::RemoveLastRunTime() {
  return RemoveGoogleUpdateStrKey(google_update::kRegLastRunTimeField);
}

bool GoogleUpdateSettings::GetBrowser(std::wstring* browser) {
  return ReadGoogleUpdateStrKey(google_update::kRegBrowserField, browser);
}

bool GoogleUpdateSettings::GetLanguage(std::wstring* language) {
  return ReadGoogleUpdateStrKey(google_update::kRegLangField, language);
}

bool GoogleUpdateSettings::GetBrand(std::wstring* brand) {
  return ReadGoogleUpdateStrKey(google_update::kRegRLZBrandField, brand);
}

bool GoogleUpdateSettings::GetClient(std::wstring* client) {
  return ReadGoogleUpdateStrKey(google_update::kRegClientField, client);
}

bool GoogleUpdateSettings::SetClient(const std::wstring& client) {
  return WriteGoogleUpdateStrKey(google_update::kRegClientField, client);
}

bool GoogleUpdateSettings::GetReferral(std::wstring* referral) {
  return ReadGoogleUpdateStrKey(google_update::kRegReferralField, referral);
}

bool GoogleUpdateSettings::ClearReferral() {
  return ClearGoogleUpdateStrKey(google_update::kRegReferralField);
}

bool GoogleUpdateSettings::GetChromeChannel(std::wstring* channel) {
  std::wstring update_branch;
  if (!ReadGoogleUpdateStrKey(google_update::kRegApField, &update_branch)) {
    *channel = L"unknown";
    return false;
  }

  // Map to something pithy for human consumption. There are no rules as to
  // what the ap string can contain, but generally it will contain a number
  // followed by a dash followed by the branch name (and then some random
  // suffix). We fall back on empty string in case we fail to parse.
  // Only ever return "", "unknown", "dev" or "beta".
  if (update_branch.find(L"-beta") != std::wstring::npos)
    *channel = L"beta";
  else if (update_branch.find(L"-dev") != std::wstring::npos)
    *channel = L"dev";
  else if (update_branch.empty())
    *channel = L"";
  else
    *channel = L"unknown";

  return true;
}


