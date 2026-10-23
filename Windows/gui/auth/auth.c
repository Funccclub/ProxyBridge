// auth.c - PBKDF2-SHA256 password storage for the portable ProxyBridge build.
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>
#include "auth.h"
#include "../profile/profile.h"

#pragma comment(lib, "bcrypt.lib")

#define AUTH_SALT_BYTES  16
#define AUTH_HASH_BYTES  32
#define AUTH_ITERATIONS  100000
#define AUTH_FILE        L"auth.dat"

static wchar_t g_authPath[MAX_PATH];

static void auth_path(wchar_t* out, int cch)
{
    wchar_t base[MAX_PATH];
    PB_GetDataDir(base, MAX_PATH);
    _snwprintf_s(out, cch, _TRUNCATE, L"%s\\%s", base, AUTH_FILE);
}

static BOOL ensure_data_dir(void)
{
    wchar_t base[MAX_PATH];
    PB_GetDataDir(base, MAX_PATH);
    return CreateDirectoryW(base, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static BOOL b64_encode(const BYTE* in, DWORD inLen, char* out, int outCch)
{
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    DWORD i = 0, o = 0;
    while (i < inLen)
    {
        DWORD remain = inLen - i;
        DWORD a = in[i++];
        DWORD b = (remain > 1) ? in[i++] : 0;
        DWORD c = (remain > 2) ? in[i++] : 0;
        if (o + 4 >= (DWORD)outCch) return FALSE;
        out[o++] = tbl[(a >> 2) & 0x3F];
        out[o++] = tbl[((a << 4) | (b >> 4)) & 0x3F];
        out[o++] = (remain < 2) ? '=' : tbl[((b << 2) | (c >> 6)) & 0x3F];
        out[o++] = (remain < 3) ? '=' : tbl[c & 0x3F];
    }
    if (o < (DWORD)outCch) out[o] = 0;
    return TRUE;
}

static int b64_val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

// Reject invalid padding and check capacity before every decoded write.
static BOOL b64_decode(const char* in, BYTE* out, DWORD capacity, DWORD* outLen)
{
    size_t len = strlen(in);
    DWORD o = 0;
    *outLen = 0;
    if (!len || len % 4) return FALSE;
    for (size_t i = 0; i < len; i += 4) {
        int a = b64_val(in[i]), b = b64_val(in[i+1]);
        int c = in[i+2] == '=' ? 0 : b64_val(in[i+2]);
        int d = in[i+3] == '=' ? 0 : b64_val(in[i+3]);
        BOOL pad2 = in[i+2] == '=', pad1 = in[i+3] == '=';
        if (a < 0 || b < 0 || c < 0 || d < 0 || (pad2 && !pad1)) return FALSE;
        if ((pad1 || pad2) && i + 4 != len) return FALSE;
        if ((pad2 && (b & 15)) || (pad1 && !pad2 && (c & 3))) return FALSE;
        DWORD n = pad2 ? 1 : pad1 ? 2 : 3;
        if (n > capacity - o) return FALSE;
        out[o++] = (BYTE)((a << 2) | (b >> 4));
        if (n > 1) out[o++] = (BYTE)((b << 4) | (c >> 2));
        if (n > 2) out[o++] = (BYTE)((c << 6) | d);
    }
    *outLen = o;
    return TRUE;
}

static BOOL derive_hash(const wchar_t* password, const BYTE* salt, DWORD saltLen,
                        BYTE* hashOut, DWORD hashLen)
{
    BCRYPT_ALG_HANDLE alg = NULL;
    NTSTATUS st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (st != 0) return FALSE;

    st = BCryptDeriveKeyPBKDF2(alg, (PUCHAR)password,
                               (ULONG)(lstrlenW(password) * sizeof(wchar_t)),
                               (PUCHAR)salt, saltLen, AUTH_ITERATIONS,
                               hashOut, hashLen, 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    return st == 0;
}

static BOOL read_auth_file(char* buf, DWORD bufCch)
{
    HANDLE h = CreateFileW(g_authPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    DWORD rd = 0;
    BOOL ok = ReadFile(h, buf, bufCch - 1, &rd, NULL);
    CloseHandle(h);
    if (!ok) return FALSE;
    buf[rd] = 0;
    return rd > 0;
}

static const char* json_str(const char* json, const char* key, char* out, int outCch)
{
    char pat[64];
    _snprintf_s(pat, sizeof(pat), _TRUNCATE, "\"%s\":\"", key);
    const char* p = strstr(json, pat);
    if (!p) { if (outCch) out[0] = 0; return NULL; }
    p += strlen(pat);
    const char* e = strchr(p, '"');
    if (!e) { if (outCch) out[0] = 0; return NULL; }
    int n = (int)(e - p);
    if (n >= outCch) { if (outCch) out[0] = 0; return NULL; }
    memcpy(out, p, (size_t)n);
    out[n] = 0;
    return out;
}

void Auth_Init(void)
{
    auth_path(g_authPath, MAX_PATH);
    // Never delete a damaged credential file: that would silently reset the password.
}

BOOL Auth_HasPassword(void)
{
    DWORD attr = GetFileAttributesW(g_authPath);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL Auth_SetPassword(const wchar_t* password)
{
    if (!password || !password[0] || lstrlenW(password) < 4 || lstrlenW(password) > AUTH_PASSWORD_MAX) return FALSE;
    if (!ensure_data_dir()) return FALSE;

    BYTE salt[AUTH_SALT_BYTES], hash[AUTH_HASH_BYTES];
    if (BCryptGenRandom(NULL, salt, AUTH_SALT_BYTES, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        return FALSE;
    if (!derive_hash(password, salt, AUTH_SALT_BYTES, hash, AUTH_HASH_BYTES))
        return FALSE;

    char saltB64[64], hashB64[64];
    if (!b64_encode(salt, AUTH_SALT_BYTES, saltB64, sizeof(saltB64))) return FALSE;
    if (!b64_encode(hash, AUTH_HASH_BYTES, hashB64, sizeof(hashB64))) return FALSE;

    char json[512];
    _snprintf_s(json, sizeof(json), _TRUNCATE,
                "{\"salt\":\"%s\",\"hash\":\"%s\",\"iter\":%d}\n",
                saltB64, hashB64, AUTH_ITERATIONS);

    wchar_t tmp[MAX_PATH];
    _snwprintf_s(tmp, MAX_PATH, _TRUNCATE, L"%s.tmp", g_authPath);
    HANDLE h = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    DWORD wr = 0;
    BOOL ok = WriteFile(h, json, (DWORD)strlen(json), &wr, NULL) && wr == strlen(json);
    CloseHandle(h);
    if (!ok) { DeleteFileW(tmp); return FALSE; }
    if (!MoveFileExW(tmp, g_authPath, MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(tmp);
        return FALSE;
    }
    return TRUE;
}

BOOL Auth_Verify(const wchar_t* password)
{
    if (!password || !password[0] || lstrlenW(password) > AUTH_PASSWORD_MAX) return FALSE;

    char json[512];
    if (!read_auth_file(json, sizeof(json))) return FALSE;

    char saltB64[64], hashB64[64];
    if (!json_str(json, "salt", saltB64, sizeof(saltB64))) return FALSE;
    if (!json_str(json, "hash", hashB64, sizeof(hashB64))) return FALSE;

    BYTE salt[AUTH_SALT_BYTES], expect[AUTH_HASH_BYTES], got[AUTH_HASH_BYTES];
    DWORD saltLen = 0, hashLen = 0;
    if (!b64_decode(saltB64, salt, sizeof(salt), &saltLen) || saltLen != AUTH_SALT_BYTES) return FALSE;
    if (!b64_decode(hashB64, expect, sizeof(expect), &hashLen) || hashLen != AUTH_HASH_BYTES) return FALSE;
    if (!derive_hash(password, salt, saltLen, got, AUTH_HASH_BYTES)) return FALSE;

    volatile BYTE diff = 0;
    for (DWORD i = 0; i < AUTH_HASH_BYTES; i++) diff |= (got[i] ^ expect[i]);
    SecureZeroMemory(got, sizeof(got));
    SecureZeroMemory(expect, sizeof(expect));
    return diff == 0;
}
