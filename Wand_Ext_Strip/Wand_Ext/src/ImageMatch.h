// ImageMatch.h
// Header-only BMP template matching using SAD (Sum of Absolute Differences)
// Accepts 24-bit BMP files only. Compares raw BGR pixels against a game window capture.
#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>

#define HOOK_PRINTF(...) printf(__VA_ARGS__)

struct MatchResult {
    int x;              // top-left X in game client coords
    int y;              // top-left Y in game client coords
    float confidence;   // 0.0 = perfect match, higher = worse (avg SAD per pixel per channel)
};

class ImageMatch {
public:
    // ---- Template (the small image to search for) ----

    struct Template {
        std::vector<uint8_t> pixels; // raw BGR, no padding
        int width = 0;
        int height = 0;

        bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
    };

    // Load a 24-bit BMP file as a template. Returns empty template on failure.
    static Template LoadBMP(const std::string& path) {
        Template t;

        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            HOOK_PRINTF("[ImageMatch] Cannot open file: %s\n", path.c_str());
            return t;
        }

        // Read file into memory
        DWORD fileSize = GetFileSize(hFile, nullptr);
        if (fileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
            HOOK_PRINTF("[ImageMatch] File too small: %s\n", path.c_str());
            CloseHandle(hFile);
            return t;
        }

        std::vector<uint8_t> fileData(fileSize);
        DWORD bytesRead = 0;
        if (!ReadFile(hFile, fileData.data(), fileSize, &bytesRead, nullptr) || bytesRead != fileSize) {
            HOOK_PRINTF("[ImageMatch] Failed to read file: %s\n", path.c_str());
            CloseHandle(hFile);
            return t;
        }
        CloseHandle(hFile);

        // Parse BMP headers
        auto* fh = reinterpret_cast<BITMAPFILEHEADER*>(fileData.data());
        if (fh->bfType != 0x4D42) { // 'BM'
            HOOK_PRINTF("[ImageMatch] Not a BMP file: %s\n", path.c_str());
            return t;
        }

        auto* ih = reinterpret_cast<BITMAPINFOHEADER*>(fileData.data() + sizeof(BITMAPFILEHEADER));

        if (ih->biBitCount != 24) {
            HOOK_PRINTF("[ImageMatch] Only 24-bit BMP supported (got %d-bit): %s\n", ih->biBitCount, path.c_str());
            return t;
        }
        if (ih->biCompression != BI_RGB) {
            HOOK_PRINTF("[ImageMatch] Only uncompressed BMP supported: %s\n", path.c_str());
            return t;
        }

        t.width = ih->biWidth;
        t.height = std::abs(ih->biHeight);
        bool topDown = (ih->biHeight < 0);

        if (t.width <= 0 || t.height <= 0) {
            HOOK_PRINTF("[ImageMatch] Invalid BMP dimensions: %dx%d\n", t.width, t.height);
            t.width = t.height = 0;
            return t;
        }

        // BMP rows are padded to 4-byte boundary
        int srcRowSize = ((t.width * 3 + 3) & ~3);
        uint8_t* pixelData = fileData.data() + fh->bfOffBits;

        // Store as tightly packed BGR (no padding), top-to-bottom
        t.pixels.resize(t.width * t.height * 3);

        for (int row = 0; row < t.height; row++) {
            // BMP default is bottom-up; we always store top-down
            int srcRow = topDown ? row : (t.height - 1 - row);
            const uint8_t* src = pixelData + srcRow * srcRowSize;
            uint8_t* dst = t.pixels.data() + row * t.width * 3;
            memcpy(dst, src, t.width * 3);
        }

        HOOK_PRINTF("[ImageMatch] Loaded template: %s (%dx%d)\n", path.c_str(), t.width, t.height);
        return t;
    }

    // ---- Game window capture (raw BGR pixels) ----

    struct Capture {
        std::vector<uint8_t> pixels; // tightly packed BGR, top-down
        int width = 0;
        int height = 0;

        bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
    };

    // Capture the game window client area as raw 24-bit BGR pixels.
    // No compression, no color conversion.
    static Capture CaptureWindow(HWND hWnd) {
        Capture cap;

        if (!hWnd || !IsWindow(hWnd))
            return cap;

        RECT rect;
        if (!GetClientRect(hWnd, &rect))
            return cap;

        cap.width = rect.right - rect.left;
        cap.height = rect.bottom - rect.top;

        if (cap.width <= 0 || cap.height <= 0) {
            cap.width = cap.height = 0;
            return cap;
        }

        HDC hdcWindow = GetDC(hWnd);
        HDC hdcMem = CreateCompatibleDC(hdcWindow);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, cap.width, cap.height);
        HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);

        BOOL captured = PrintWindow(hWnd, hdcMem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
        if (!captured) {
            BitBlt(hdcMem, 0, 0, cap.width, cap.height, hdcWindow, 0, 0, SRCCOPY);
        }

        // Extract as 24-bit BGR, top-down (negative height)
        BITMAPINFOHEADER bi = {};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = cap.width;
        bi.biHeight = -cap.height; // top-down
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;

        int rowSize = ((cap.width * 3 + 3) & ~3);
        int imageSize = rowSize * cap.height;

        std::vector<uint8_t> rawPixels(imageSize);
        GetDIBits(hdcMem, hBitmap, 0, cap.height, rawPixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        SelectObject(hdcMem, hOld);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hWnd, hdcWindow);

        // Remove row padding -> tightly packed BGR
        cap.pixels.resize(cap.width * cap.height * 3);
        for (int row = 0; row < cap.height; row++) {
            const uint8_t* src = rawPixels.data() + row * rowSize;
            uint8_t* dst = cap.pixels.data() + row * cap.width * 3;
            memcpy(dst, src, cap.width * 3);
        }

        return cap;
    }

    // ---- SAD Template Matching ----

    // Find all locations where the template matches within the threshold.
    // threshold: max average SAD per pixel per channel (0 = exact match only, 10 = lenient).
    // Returns matches sorted by confidence (best first).
    static std::vector<MatchResult> FindAll(const Capture& capture, const Template& tmpl, float threshold) {
        std::vector<MatchResult> results;

        if (!capture.IsValid() || !tmpl.IsValid())
            return results;

        if (tmpl.width > capture.width || tmpl.height > capture.height)
            return results;

        int searchW = capture.width - tmpl.width + 1;
        int searchH = capture.height - tmpl.height + 1;
        int tmplPixels = tmpl.width * tmpl.height;
        int tmplStride = tmpl.width * 3;
        int capStride = capture.width * 3;

        for (int sy = 0; sy < searchH; sy++) {
            for (int sx = 0; sx < searchW; sx++) {
                long long totalSAD = 0;

                for (int ty = 0; ty < tmpl.height; ty++) {
                    const uint8_t* capRow = capture.pixels.data() + (sy + ty) * capStride + sx * 3;
                    const uint8_t* tmplRow = tmpl.pixels.data() + ty * tmplStride;

                    for (int tx = 0; tx < tmplStride; tx++) {
                        totalSAD += std::abs(static_cast<int>(capRow[tx]) - static_cast<int>(tmplRow[tx]));
                    }
                }

                // Average SAD per pixel per channel (3 channels)
                float avgSAD = static_cast<float>(totalSAD) / static_cast<float>(tmplPixels * 3);

                if (avgSAD <= threshold) {
                    results.push_back({ sx, sy, avgSAD });
                }
            }
        }

        // Sort by confidence (lowest SAD = best match first)
        std::sort(results.begin(), results.end(),
            [](const MatchResult& a, const MatchResult& b) { return a.confidence < b.confidence; });

        return results;
    }

    // Find the single best match. Returns a match with confidence = -1 if nothing found within threshold.
    static MatchResult FindBest(const Capture& capture, const Template& tmpl, float threshold) {
        MatchResult best = { 0, 0, -1.0f };

        if (!capture.IsValid() || !tmpl.IsValid())
            return best;

        if (tmpl.width > capture.width || tmpl.height > capture.height)
            return best;

        int searchW = capture.width - tmpl.width + 1;
        int searchH = capture.height - tmpl.height + 1;
        int tmplPixels = tmpl.width * tmpl.height;
        int tmplStride = tmpl.width * 3;
        int capStride = capture.width * 3;

        float bestSAD = threshold + 1.0f; // anything above threshold

        for (int sy = 0; sy < searchH; sy++) {
            for (int sx = 0; sx < searchW; sx++) {
                long long totalSAD = 0;
                bool earlyExit = false;

                // Early exit: if partial SAD already exceeds best, skip
                long long maxSAD = static_cast<long long>(bestSAD * tmplPixels * 3);

                for (int ty = 0; ty < tmpl.height && !earlyExit; ty++) {
                    const uint8_t* capRow = capture.pixels.data() + (sy + ty) * capStride + sx * 3;
                    const uint8_t* tmplRow = tmpl.pixels.data() + ty * tmplStride;

                    for (int tx = 0; tx < tmplStride; tx++) {
                        totalSAD += std::abs(static_cast<int>(capRow[tx]) - static_cast<int>(tmplRow[tx]));
                    }

                    if (totalSAD > maxSAD) {
                        earlyExit = true;
                    }
                }

                if (!earlyExit) {
                    float avgSAD = static_cast<float>(totalSAD) / static_cast<float>(tmplPixels * 3);
                    if (avgSAD < bestSAD) {
                        bestSAD = avgSAD;
                        best = { sx, sy, avgSAD };
                    }
                }
            }
        }

        // Check if we actually found something within threshold
        if (best.confidence < 0 || best.confidence > threshold)
            best.confidence = -1.0f;

        return best;
    }

    // ---- Convenience: capture + search in one call ----

    static MatchResult FindInWindow(HWND hWnd, const Template& tmpl, float threshold) {
        auto cap = CaptureWindow(hWnd);
        return FindBest(cap, tmpl, threshold);
    }

    static std::vector<MatchResult> FindAllInWindow(HWND hWnd, const Template& tmpl, float threshold) {
        auto cap = CaptureWindow(hWnd);
        return FindAll(cap, tmpl, threshold);
    }
};
