// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef OS_WINDOWS_SETUP_H_
#define OS_WINDOWS_SETUP_H_

#include <WinSdkVer.h>
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#ifdef WINVER
#undef WINVER
#endif
#define _WIN32_WINNT _WIN32_WINNT_WINBLUE
#define WINVER _WIN32_WINNT

// Must be included first because Windows headers don't include what they use.
#include <Windows.h>

#endif
