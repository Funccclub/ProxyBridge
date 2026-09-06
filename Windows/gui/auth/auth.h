// auth.h - local password gate (stored beside portable data dir).
#ifndef PB_AUTH_H
#define PB_AUTH_H

#include <windows.h>

// Initialize auth storage path (call after PB_InitStorage).
void Auth_Init(void);

// TRUE if a password has been configured.
BOOL Auth_HasPassword(void);

// Set password on first run. Returns FALSE on I/O or validation error.
BOOL Auth_SetPassword(const wchar_t* password);

// Verify password. Returns FALSE if wrong or missing file.
BOOL Auth_Verify(const wchar_t* password);

#endif // PB_AUTH_H
