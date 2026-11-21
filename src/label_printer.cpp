#include "label_printer.h"

#include <objidl.h>
#include <ole2.h>
#include <gdiplus.h>
#include <commdlg.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

namespace {

BYTE ClampByte(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<BYTE>(v);
}

std::wstring ToUpperW(const std::wstring& s) {
    if (s.empty()) return s;
    std::vector<wchar_t> buf(s.begin(), s.end());
    buf.push_back(L'\0');
    CharUpperBuffW(buf.data(), static_cast<DWORD>(s.size()));
    return std::wstring(buf.data(), s.size());
}

void AddRoundedRect(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float r) {
    if (r <= 0.5f) {
        path.AddRectangle(Gdiplus::RectF(x, y, w, h));
        return;
    }
    float maxR = (w < h ? w : h) * 0.5f;
    if (r > maxR) r = maxR;
    float d = r * 2.0f;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void AddCyberHud(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float bevel) {
    float maxBevel = (w < h ? w : h) * 0.25f;
    if (bevel > maxBevel) bevel = maxBevel;
    if (bevel < 3.0f) bevel = 3.0f;

    Gdiplus::PointF pts[6] = {
        Gdiplus::PointF(x + bevel, y),
        Gdiplus::PointF(x + w, y),
        Gdiplus::PointF(x + w, y + h - bevel),
        Gdiplus::PointF(x + w - bevel, y + h),
        Gdiplus::PointF(x, y + h),
        Gdiplus::PointF(x, y + bevel)
    };
    path.AddPolygon(pts, 6);
}

// Measure exact visual ink bounding box of text using GraphicsPath::AddString
bool MeasureInkBounds(const Gdiplus::FontFamily* family,
                      INT fontStyle,
                      float emSizePx,
                      const std::wstring& text,
                      Gdiplus::RectF& outBounds) {
    Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
    sf.SetFormatFlags(sf.GetFormatFlags() |
                      Gdiplus::StringFormatFlagsMeasureTrailingSpaces |
                      Gdiplus::StringFormatFlagsNoWrap);
    Gdiplus::GraphicsPath path;
    path.AddString(text.c_str(), -1, family, fontStyle, emSizePx, Gdiplus::PointF(0.0f, 0.0f), &sf);
    if (path.GetPointCount() == 0) {
        return false;
    }
    path.GetBounds(&outBounds);
    return (outBounds.Width > 0.1f && outBounds.Height > 0.1f);
}

// Measure & fit a font size (in pixels) so the ACTUAL ink bounds fill (maxW, maxH) tightly
float ComputeFittedFontPx(Gdiplus::Graphics& /*g*/,
                          const Gdiplus::FontFamily* family,
                          INT fontStyle,
                          const std::wstring& text,
                          float maxW,
                          float maxH,
                          float minPx) {
    if (text.empty() || maxW <= 4.0f || maxH <= 4.0f) {
        return minPx;
    }

    float lo = minPx;
    float hi = maxH * 1.9f;
    if (hi < lo) hi = lo;

    for (int iter = 0; iter < 15; ++iter) {
        float mid = (lo + hi) * 0.5f;
        Gdiplus::RectF bounds;
        if (MeasureInkBounds(family, fontStyle, mid, text, bounds) &&
            bounds.Width <= maxW && bounds.Height <= maxH) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;
}

void DrawInkCenteredText(Gdiplus::Graphics& g,
                         const Gdiplus::FontFamily* family,
                         INT fontStyle,
                         float emSizePx,
                         const std::wstring& text,
                         float boxLeft,
                         float boxTop,
                         float boxW,
                         float boxH,
                         const Gdiplus::Color& fillColor,
                         bool withShadow,
                         COLORREF shadowColor) {
    Gdiplus::RectF ink;
    if (!MeasureInkBounds(family, fontStyle, emSizePx, text, ink)) {
        return;
    }

    float drawX = boxLeft + (boxW - ink.Width) * 0.5f - ink.X;
    float drawY = boxTop + (boxH - ink.Height) * 0.5f - ink.Y;

    Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
    sf.SetFormatFlags(sf.GetFormatFlags() |
                      Gdiplus::StringFormatFlagsMeasureTrailingSpaces |
                      Gdiplus::StringFormatFlagsNoWrap);

    if (withShadow) {
        float shOff = emSizePx * 0.032f;
        if (shOff < 1.5f) shOff = 1.5f;
        Gdiplus::GraphicsPath shPath;
        shPath.AddString(text.c_str(), -1, family, fontStyle, emSizePx, Gdiplus::PointF(drawX + shOff, drawY + shOff), &sf);
        Gdiplus::SolidBrush shBrush(Gdiplus::Color(
            185, GetRValue(shadowColor), GetGValue(shadowColor), GetBValue(shadowColor)));
        g.FillPath(&shBrush, &shPath);
    }

    Gdiplus::GraphicsPath textPath;
    textPath.AddString(text.c_str(), -1, family, fontStyle, emSizePx, Gdiplus::PointF(drawX, drawY), &sf);
    Gdiplus::SolidBrush textBrush(fillColor);
    g.FillPath(&textBrush, &textPath);
}

void DrawCrispLogo(Gdiplus::Graphics& g,
                   Gdiplus::Image* logoImg,
                   float x,
                   float y,
                   float boxW,
                   float boxH) {
    if (!logoImg || logoImg->GetLastStatus() != Gdiplus::Ok ||
        logoImg->GetWidth() == 0 || logoImg->GetHeight() == 0 ||
        boxW <= 2.0f || boxH <= 2.0f) {
        return;
    }

    float imgW = static_cast<float>(logoImg->GetWidth());
    float imgH = static_cast<float>(logoImg->GetHeight());
    float aspect = imgW / imgH;

    float drawH = boxH;
    float drawW = drawH * aspect;
    if (drawW > boxW) {
        drawW = boxW;
        drawH = drawW / aspect;
    }

    float drawX = x + (boxW - drawW) * 0.5f;
    float drawY = y + (boxH - drawH) * 0.5f;

    Gdiplus::GraphicsState state = g.Save();
    Gdiplus::ImageAttributes ia;
    ia.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.DrawImage(
        logoImg,
        Gdiplus::RectF(drawX, drawY, drawW, drawH),
        0.0f, 0.0f, imgW, imgH,
        Gdiplus::UnitPixel,
        &ia);
    g.Restore(state);
}

void DrawLabelCard(Gdiplus::Graphics& g,
                   float x,
                   float y,
                   float w,
                   float h,
                   const OverlayConfig& style,
                   const std::wstring& mainText,
                   const std::wstring& subText,
                   Gdiplus::Image* logoImg) {
    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    float unit = (w < h ? w : h);
    float pad = unit * 0.022f;
    float px = x + pad;
    float py = y + pad;
    float pw = w - pad * 2.0f;
    float ph = h - pad * 2.0f;
    if (pw < 10.0f || ph < 10.0f) return;

    BYTE accR = GetRValue(style.accentColor);
    BYTE accG = GetGValue(style.accentColor);
    BYTE accB = GetBValue(style.accentColor);
    BYTE bgR = GetRValue(style.backgroundColor);
    BYTE bgG = GetGValue(style.backgroundColor);
    BYTE bgB = GetBValue(style.backgroundColor);
    BYTE txtR = GetRValue(style.textColor);
    BYTE txtG = GetGValue(style.textColor);
    BYTE txtB = GetBValue(style.textColor);

    bool isCyber = (style.frameStyle == FrameStyle::Cyber);
    bool isPill = (style.frameStyle == FrameStyle::Pill);
    bool isMinimal = (style.frameStyle == FrameStyle::None);

    // For printing on white paper, Minimal (no bg) with light text needs dark text or subtle dark frame
    bool useDarkTextOnWhite = false;
    if (isMinimal) {
        int textLuma = (299 * txtR + 587 * txtG + 114 * txtB) / 1000;
        if (textLuma > 165) {
            useDarkTextOnWhite = true;
        }
    }

    float leftBarSpace = 0.0f;

    if (isCyber) {
        float bevel = unit * 0.12f;
        Gdiplus::GraphicsPath hudPath;
        AddCyberHud(hudPath, px, py, pw, ph, bevel);

        // Soft neon outer aura
        Gdiplus::Pen glow1(Gdiplus::Color(42, accR, accG, accB), unit * 0.032f);
        glow1.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawPath(&glow1, &hudPath);

        // Rich solid dark carbon gradient (full opacity for vivid printing)
        Gdiplus::Color topCol(
            255,
            ClampByte(static_cast<int>(bgR) + 16),
            ClampByte(static_cast<int>(bgG) + 20),
            ClampByte(static_cast<int>(bgB) + 28));
        Gdiplus::Color botCol(255, bgR, bgG, bgB);
        Gdiplus::LinearGradientBrush bgBrush(
            Gdiplus::PointF(px, py),
            Gdiplus::PointF(px, py + ph),
            topCol,
            botCol);
        g.FillPath(&bgBrush, &hudPath);

        // Ambient accent wash on left + subtle top sheen
        g.SetClip(&hudPath);
        {
            Gdiplus::LinearGradientBrush sideGlow(
                Gdiplus::PointF(px, py),
                Gdiplus::PointF(px + pw * 0.36f, py),
                Gdiplus::Color(36, accR, accG, accB),
                Gdiplus::Color(0, accR, accG, accB));
            g.FillRectangle(&sideGlow, px, py, pw * 0.36f, ph);

            Gdiplus::LinearGradientBrush sheen(
                Gdiplus::PointF(px, py),
                Gdiplus::PointF(px, py + ph * 0.45f),
                Gdiplus::Color(22, 255, 255, 255),
                Gdiplus::Color(0, 255, 255, 255));
            g.FillRectangle(&sheen, px, py, pw, ph * 0.45f);
        }
        g.ResetClip();

        // Crisp neon border
        Gdiplus::Pen borderPen(Gdiplus::Color(245, accR, accG, accB), (unit * 0.014f > 1.5f ? unit * 0.014f : 1.5f));
        borderPen.SetLineJoin(Gdiplus::LineJoinMiter);
        g.DrawPath(&borderPen, &hudPath);

        // Left glowing accent strip
        float barX = px + unit * 0.065f;
        float barY = py + bevel * 0.82f;
        float barW = unit * 0.028f;
        if (barW < 2.5f) barW = 2.5f;
        float barH = ph - bevel * 0.82f - unit * 0.065f;
        if (barH > 6.0f) {
            Gdiplus::GraphicsPath barPath;
            AddRoundedRect(barPath, barX, barY, barW, barH, barW * 0.5f);
            Gdiplus::SolidBrush barBrush(Gdiplus::Color(255, accR, accG, accB));
            g.FillPath(&barBrush, &barPath);
        }

        // Top-right HUD bracket highlight
        float brk = unit * 0.10f;
        Gdiplus::Pen brkPen(
            Gdiplus::Color(255,
                           ClampByte((static_cast<int>(accR) + 255) / 2),
                           ClampByte((static_cast<int>(accG) + 255) / 2),
                           ClampByte((static_cast<int>(accB) + 255) / 2)),
            (unit * 0.020f > 2.0f ? unit * 0.020f : 2.0f));
        g.DrawLine(&brkPen, px + pw - brk, py + 1.0f, px + pw - 1.0f, py + 1.0f);
        g.DrawLine(&brkPen, px + pw - 1.0f, py + 1.0f, px + pw - 1.0f, py + brk);

        leftBarSpace = unit * 0.09f;
    } else if (isPill) {
        Gdiplus::GraphicsPath bgPath;
        AddRoundedRect(bgPath, px, py, pw, ph, unit * 0.14f);
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(245, bgR, bgG, bgB));
        g.FillPath(&bgBrush, &bgPath);

        Gdiplus::Pen borderPen(Gdiplus::Color(210, accR, accG, accB), (unit * 0.015f > 1.5f ? unit * 0.015f : 1.5f));
        g.DrawPath(&borderPen, &bgPath);
    } else {
        // Minimal clean card on white paper with crisp accent outline
        Gdiplus::GraphicsPath bgPath;
        AddRoundedRect(bgPath, px, py, pw, ph, unit * 0.08f);
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 252, 253, 255));
        g.FillPath(&bgBrush, &bgPath);

        Gdiplus::Pen borderPen(Gdiplus::Color(220, 40, 52, 70), (unit * 0.014f > 1.5f ? unit * 0.014f : 1.5f));
        g.DrawPath(&borderPen, &bgPath);
    }

    // Font family
    Gdiplus::FontFamily requestedFamily(style.fontName.c_str());
    Gdiplus::FontFamily fallbackFamily(L"Segoe UI");
    const Gdiplus::FontFamily* activeFamily =
        (requestedFamily.GetLastStatus() == Gdiplus::Ok) ? &requestedFamily : &fallbackFamily;

    float innerLeft = px + leftBarSpace + pw * 0.045f;
    float innerRight = px + pw - pw * 0.045f;
    float innerTop = py + ph * 0.080f;
    float innerBottom = py + ph - ph * 0.065f;
    float availW = innerRight - innerLeft;
    float availH = innerBottom - innerTop;
    if (availW < 10.0f || availH < 10.0f) return;

    // Clip all interior elements strictly inside the card panel so nothing can ever overflow
    Gdiplus::GraphicsState cardClipState = g.Save();
    if (isCyber) {
        float bevel = unit * 0.12f;
        Gdiplus::GraphicsPath clipPath;
        AddCyberHud(clipPath, px + 2.0f, py + 2.0f, pw - 4.0f, ph - 4.0f, bevel);
        g.SetClip(&clipPath, Gdiplus::CombineModeIntersect);
    } else if (isPill) {
        Gdiplus::GraphicsPath clipPath;
        AddRoundedRect(clipPath, px + 2.0f, py + 2.0f, pw - 4.0f, ph - 4.0f, unit * 0.14f);
        g.SetClip(&clipPath, Gdiplus::CombineModeIntersect);
    }

    bool hasLogo = (logoImg != nullptr && logoImg->GetLastStatus() == Gdiplus::Ok &&
                    logoImg->GetWidth() > 0 && logoImg->GetHeight() > 0);
    bool hasSub = !subText.empty();
    bool hasHeader = hasLogo || hasSub;
    bool darkCard = !useDarkTextOnWhite;

    float mainAreaTop = innerTop;
    float mainAreaH = availH;

    if (hasHeader) {
        // Balanced header band (~30% height when logo present) with generous safe margins so logo never overflows
        float headerH = availH * (hasLogo ? 0.30f : 0.23f);

        float logoDrawW = 0.0f;
        float logoDrawH = headerH * 0.90f;
        float logoGap = 0.0f;

        if (hasLogo) {
            float aspect = static_cast<float>(logoImg->GetWidth()) / static_cast<float>(logoImg->GetHeight());
            float maxLogoW = hasSub ? (availW * 0.28f) : (availW * 0.48f);
            logoDrawW = logoDrawH * aspect;
            if (logoDrawW > maxLogoW) {
                logoDrawW = maxLogoW;
                logoDrawH = logoDrawW / aspect;
            }
            if (hasSub) {
                logoGap = headerH * 0.22f;
            }
        }

        if (hasSub) {
            float maxTextW = availW - logoDrawW - logoGap;
            if (maxTextW < 10.0f) maxTextW = 10.0f;
            float subFontPx = ComputeFittedFontPx(
                g, activeFamily, Gdiplus::FontStyleBold,
                subText, maxTextW * 0.94f, headerH * 0.62f, 10.0f);

            Gdiplus::RectF subInk;
            MeasureInkBounds(activeFamily, Gdiplus::FontStyleBold, subFontPx, subText, subInk);

            float totalHeaderW = logoDrawW + logoGap + subInk.Width;
            if (totalHeaderW > availW) {
                totalHeaderW = availW;
            }
            float cursorX = innerLeft + (availW - totalHeaderW) * 0.5f;

            if (hasLogo) {
                float ly = innerTop + (headerH - logoDrawH) * 0.5f;
                DrawCrispLogo(g, logoImg, cursorX, ly, logoDrawW, logoDrawH);
                cursorX += logoDrawW + logoGap;
            }

            Gdiplus::Color subColor = useDarkTextOnWhite
                ? Gdiplus::Color(235, 38, 62, 86)
                : Gdiplus::Color(
                      242,
                      ClampByte((static_cast<int>(accR) * 55 + 255 * 45) / 100),
                      ClampByte((static_cast<int>(accG) * 55 + 255 * 45) / 100),
                      ClampByte((static_cast<int>(accB) * 55 + 255 * 45) / 100));

            DrawInkCenteredText(
                g, activeFamily, Gdiplus::FontStyleBold, subFontPx, subText,
                cursorX, innerTop, subInk.Width, headerH, subColor, false, style.shadowColor);
        } else if (hasLogo) {
            float lx = innerLeft + (availW - logoDrawW) * 0.5f;
            float ly = innerTop + (headerH - logoDrawH) * 0.5f;
            DrawCrispLogo(g, logoImg, lx, ly, logoDrawW, logoDrawH);
        }

        // Crisp subtle divider under header band
        float sepY = innerTop + headerH + availH * 0.035f;
        Gdiplus::Pen sepPen(
            darkCard ? Gdiplus::Color(115, accR, accG, accB) : Gdiplus::Color(75, 40, 52, 70),
            (unit * 0.007f > 1.2f ? unit * 0.007f : 1.2f));
        g.DrawLine(&sepPen, innerLeft + availW * 0.02f, sepY, innerRight - availW * 0.02f, sepY);

        mainAreaTop = sepY + availH * 0.035f;
        mainAreaH = innerBottom - mainAreaTop;
    }

    // Giant, optically dead-centered machine number filling the full lower card for long-distance legibility
    INT mainStyle = style.bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
    float mainFontPx = ComputeFittedFontPx(
        g, activeFamily, mainStyle, mainText, availW * 0.94f, mainAreaH * 0.84f, 12.0f);

    Gdiplus::Color mainColor = useDarkTextOnWhite
        ? Gdiplus::Color(255, 18, 26, 40)
        : Gdiplus::Color(255, txtR, txtG, txtB);

    DrawInkCenteredText(
        g, activeFamily, mainStyle, mainFontPx, mainText,
        innerLeft, mainAreaTop, availW, mainAreaH,
        mainColor, style.shadow && !useDarkTextOnWhite, style.shadowColor);

    g.Restore(cardClipState);
}

void DrawA4Page(Gdiplus::Graphics& g,
                int pageW,
                int pageH,
                const OverlayConfig& style,
                const PrintJobConfig& job,
                int pageStartIndex,
                int totalItems) {
    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    g.Clear(Gdiplus::Color(255, 255, 255, 255));

    int cols = 2;
    int rows = 4;
    GetPrintLayoutGrid(job.layoutIndex, cols, rows);

    // Margins & gaps in proportions of A4 (210 x 297 mm)
    float pxPerMmX = static_cast<float>(pageW) / 210.0f;
    float pxPerMmY = static_cast<float>(pageH) / 297.0f;

    float marginX = 11.0f * pxPerMmX;
    float marginY = 12.0f * pxPerMmY;
    float gapX = 6.0f * pxPerMmX;
    float gapY = 6.0f * pxPerMmY;

    float cellW = (static_cast<float>(pageW) - marginX * 2.0f - gapX * static_cast<float>(cols - 1)) / static_cast<float>(cols);
    float cellH = (static_cast<float>(pageH) - marginY * 2.0f - gapY * static_cast<float>(rows - 1)) / static_cast<float>(rows);

    std::wstring subText = style.uppercase ? ToUpperW(job.subLabel) : job.subLabel;

    std::unique_ptr<Gdiplus::Image> logoImg;
    if (!style.logoPath.empty()) {
        auto img = std::make_unique<Gdiplus::Image>(style.logoPath.c_str());
        if (img->GetLastStatus() == Gdiplus::Ok && img->GetWidth() > 0 && img->GetHeight() > 0) {
            logoImg = std::move(img);
        }
    }

    Gdiplus::Pen cutPen(Gdiplus::Color(150, 150, 150, 150), (pxPerMmX * 0.30f > 1.0f ? pxPerMmX * 0.30f : 1.0f));
    cutPen.SetDashStyle(Gdiplus::DashStyleDash);

    int low = job.startNum <= job.endNum ? job.startNum : job.endNum;
    int perPage = cols * rows;

    for (int slot = 0; slot < perPage; ++slot) {
        int itemIdx = pageStartIndex + slot;
        if (itemIdx >= totalItems) break;

        int r = slot / cols;
        int c = slot % cols;
        float cx = marginX + static_cast<float>(c) * (cellW + gapX);
        float cy = marginY + static_cast<float>(r) * (cellH + gapY);

        int number = low + itemIdx;
        std::wstring labelText = FormatPrintLabelNumber(job, number, style.uppercase);

        DrawLabelCard(g, cx, cy, cellW, cellH, style, labelText, subText, logoImg.get());

        if (job.showCutLines) {
            g.DrawRectangle(&cutPen, cx, cy, cellW, cellH);
        }
    }
}

bool GetJpegEncoderClsid(CLSID& outClsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;

    std::vector<BYTE> buffer(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(num, size, codecs) != Gdiplus::Ok) {
        return false;
    }

    for (UINT i = 0; i < num; ++i) {
        if (codecs[i].MimeType && wcscmp(codecs[i].MimeType, L"image/jpeg") == 0) {
            outClsid = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

bool EncodeBitmapToJpeg(Gdiplus::Bitmap& bmp, const CLSID& jpegClsid, std::vector<BYTE>& outBytes) {
    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream)) || !pStream) {
        return false;
    }

    static const GUID kEncoderQuality =
        { 0x1d5be4b5, 0xfa4a, 0x452d, { 0x9c, 0xdd, 0x5d, 0xb3, 0x51, 0x05, 0xe7, 0xeb } };

    ULONG quality = 94;
    Gdiplus::EncoderParameters encParams = {};
    encParams.Count = 1;
    encParams.Parameter[0].Guid = kEncoderQuality;
    encParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    encParams.Parameter[0].NumberOfValues = 1;
    encParams.Parameter[0].Value = &quality;

    Gdiplus::Status st = bmp.Save(pStream, &jpegClsid, &encParams);
    if (st != Gdiplus::Ok) {
        pStream->Release();
        return false;
    }

    HGLOBAL hMem = nullptr;
    if (FAILED(GetHGlobalFromStream(pStream, &hMem)) || !hMem) {
        pStream->Release();
        return false;
    }

    STATSTG stat = {};
    pStream->Stat(&stat, STATFLAG_NONAME);
    size_t dataSize = static_cast<size_t>(stat.cbSize.QuadPart);

    void* ptr = GlobalLock(hMem);
    if (!ptr || dataSize == 0) {
        if (ptr) GlobalUnlock(hMem);
        pStream->Release();
        return false;
    }

    outBytes.resize(dataSize);
    std::memcpy(outBytes.data(), ptr, dataSize);
    GlobalUnlock(hMem);
    pStream->Release();
    return true;
}

bool WriteJpegPagesToPdf(const std::wstring& pdfPath,
                         const std::vector<std::vector<BYTE>>& jpegPages,
                         int imgW,
                         int imgH) {
    HANDLE hFile = CreateFileW(pdfPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::string pdf;
    pdf.reserve(jpegPages.size() * 256 * 1024);

    // Binary PDF 1.4 header
    pdf += "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";

    // Object numbering:
    // 1 = Catalog
    // 2 = Pages
    // For each page i (0..N-1):
    //   pageObj    = 3 + i * 3
    //   contentObj = 4 + i * 3
    //   imageObj   = 5 + i * 3
    const int nPages = static_cast<int>(jpegPages.size());
    const int totalObjs = 2 + nPages * 3;
    std::vector<size_t> offsets(static_cast<size_t>(totalObjs) + 1, 0);

    // Object 1: Catalog
    offsets[1] = pdf.size();
    pdf += "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";

    // Object 2: Pages
    offsets[2] = pdf.size();
    pdf += "2 0 obj\n<< /Type /Pages /Kids [";
    for (int i = 0; i < nPages; ++i) {
        pdf += " " + std::to_string(3 + i * 3) + " 0 R";
    }
    pdf += " ] /Count " + std::to_string(nPages) + " >>\nendobj\n";

    // A4 dimensions in PDF points (1/72 inch): 210mm x 297mm = 595.28 x 841.89 pt
    for (int i = 0; i < nPages; ++i) {
        int pageObj = 3 + i * 3;
        int contentObj = 4 + i * 3;
        int imageObj = 5 + i * 3;

        // Page object
        offsets[static_cast<size_t>(pageObj)] = pdf.size();
        {
            std::ostringstream oss;
            oss << pageObj << " 0 obj\n"
                << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595.28 841.89] "
                << "/Resources << /XObject << /Im0 " << imageObj << " 0 R >> >> "
                << "/Contents " << contentObj << " 0 R >>\n"
                << "endobj\n";
            pdf += oss.str();
        }

        // Content stream
        std::string contentStream = "q\n595.28 0 0 841.89 0 0 cm\n/Im0 Do\nQ\n";
        offsets[static_cast<size_t>(contentObj)] = pdf.size();
        {
            std::ostringstream oss;
            oss << contentObj << " 0 obj\n"
                << "<< /Length " << contentStream.size() << " >>\n"
                << "stream\n" << contentStream << "endstream\n"
                << "endobj\n";
            pdf += oss.str();
        }

        // Image XObject (raw JPEG /DCTDecode)
        const auto& jpg = jpegPages[static_cast<size_t>(i)];
        offsets[static_cast<size_t>(imageObj)] = pdf.size();
        {
            std::ostringstream oss;
            oss << imageObj << " 0 obj\n"
                << "<< /Type /XObject /Subtype /Image "
                << "/Width " << imgW << " /Height " << imgH << " "
                << "/ColorSpace /DeviceRGB /BitsPerComponent 8 "
                << "/Filter /DCTDecode /Length " << jpg.size() << " >>\n"
                << "stream\n";
            pdf += oss.str();
        }
        pdf.append(reinterpret_cast<const char*>(jpg.data()), jpg.size());
        pdf += "\nendstream\nendobj\n";
    }

    // xref table
    size_t xrefPos = pdf.size();
    pdf += "xref\n0 " + std::to_string(totalObjs + 1) + "\n";
    pdf += "0000000000 65535 f \n";
    for (int i = 1; i <= totalObjs; ++i) {
        char line[32] = {};
        std::snprintf(line, sizeof(line), "%010zu 00000 n \n", offsets[static_cast<size_t>(i)]);
        pdf += line;
    }

    // trailer
    {
        std::ostringstream oss;
        oss << "trailer\n<< /Size " << (totalObjs + 1) << " /Root 1 0 R >>\n"
            << "startxref\n" << xrefPos << "\n%%EOF\n";
        pdf += oss.str();
    }

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, pdf.data(), static_cast<DWORD>(pdf.size()), &written, nullptr);
    CloseHandle(hFile);
    return ok && written == static_cast<DWORD>(pdf.size());
}

} // namespace

void GetPrintLayoutGrid(int layoutIndex, int& outCols, int& outRows) {
    switch (layoutIndex) {
        case 1: outCols = 1; outRows = 3; break; // Cỡ Đại: 3 nhãn/trang A4 (~180 x 85 mm)
        case 2: outCols = 2; outRows = 5; break; // Cỡ Vừa: 10 nhãn/trang A4 (~90 x 52 mm)
        case 3: outCols = 3; outRows = 6; break; // Cỡ Gọn: 18 nhãn/trang A4 (~60 x 42 mm)
        case 0:
        default:
            outCols = 2; outRows = 4; break;     // Cỡ Lớn (chuẩn phòng máy): 8 nhãn/trang A4 (~90 x 65 mm)
    }
}

std::wstring FormatPrintLabelNumber(const PrintJobConfig& job, int num, bool uppercase) {
    int hi = job.startNum >= job.endNum ? job.startNum : job.endNum;
    int width = 2;
    if (hi >= 1000) width = 4;
    else if (hi >= 100) width = 3;

    wchar_t numBuf[32] = {};
    if (job.padZero) {
        std::swprintf(numBuf, 32, L"%0*d", width, num);
    } else {
        std::swprintf(numBuf, 32, L"%d", num);
    }

    std::wstring result;
    if (!job.prefix.empty()) {
        result = job.prefix + job.separator + numBuf;
    } else {
        result = numBuf;
    }

    if (uppercase) {
        result = ToUpperW(result);
    }
    return result;
}

void RenderPrintPreviewCard(HDC hdc, const RECT& rc, const OverlayConfig& style, const PrintJobConfig& job) {
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 4 || h <= 4) return;

    Gdiplus::Bitmap bmp(w, h, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);
    g.Clear(Gdiplus::Color(255, 245, 247, 250));

    // Preview white paper backdrop + centered label card with authentic aspect ratio
    int cols = 2, rows = 4;
    GetPrintLayoutGrid(job.layoutIndex, cols, rows);
    float cardAspect = (188.0f / static_cast<float>(cols)) / (273.0f / static_cast<float>(rows));

    float maxW = static_cast<float>(w) * 0.90f;
    float maxH = static_cast<float>(h) * 0.84f;
    float drawW = maxW;
    float drawH = drawW / cardAspect;
    if (drawH > maxH) {
        drawH = maxH;
        drawW = drawH * cardAspect;
    }

    float drawX = (static_cast<float>(w) - drawW) * 0.5f;
    float drawY = (static_cast<float>(h) - drawH) * 0.5f;

    std::unique_ptr<Gdiplus::Image> logoImg;
    if (!style.logoPath.empty()) {
        auto img = std::make_unique<Gdiplus::Image>(style.logoPath.c_str());
        if (img->GetLastStatus() == Gdiplus::Ok && img->GetWidth() > 0 && img->GetHeight() > 0) {
            logoImg = std::move(img);
        }
    }

    int firstNum = job.startNum <= job.endNum ? job.startNum : job.endNum;
    std::wstring sampleMain = FormatPrintLabelNumber(job, firstNum, style.uppercase);
    std::wstring sampleSub = style.uppercase ? ToUpperW(job.subLabel) : job.subLabel;

    DrawLabelCard(g, drawX, drawY, drawW, drawH, style, sampleMain, sampleSub, logoImg.get());

    if (job.showCutLines) {
        Gdiplus::Pen cutPen(Gdiplus::Color(160, 140, 140, 140), 1.0f);
        cutPen.SetDashStyle(Gdiplus::DashStyleDash);
        g.DrawRectangle(&cutPen, drawX, drawY, drawW, drawH);
    }

    Gdiplus::Graphics outG(hdc);
    outG.DrawImage(&bmp, static_cast<INT>(rc.left), static_cast<INT>(rc.top));
}

void RenderPrintA4PagePreview(HDC hdc, const RECT& rc, const OverlayConfig& style, const PrintJobConfig& job, int pageIndex) {
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 16 || h <= 16) return;

    Gdiplus::Bitmap canvas(w, h, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&canvas);
    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.Clear(Gdiplus::Color(255, 224, 228, 235));

    // A4 aspect ratio (210 / 297) centered with crisp drop shadow
    const float a4Aspect = 210.0f / 297.0f;
    float pad = 14.0f;
    float maxW = static_cast<float>(w) - pad * 2.0f;
    float maxH = static_cast<float>(h) - pad * 2.0f;
    float paperH = maxH;
    float paperW = paperH * a4Aspect;
    if (paperW > maxW) {
        paperW = maxW;
        paperH = paperW / a4Aspect;
    }

    int px = static_cast<int>(std::round((static_cast<float>(w) - paperW) * 0.5f));
    int py = static_cast<int>(std::round((static_cast<float>(h) - paperH) * 0.5f));
    int pw = static_cast<int>(std::round(paperW));
    int ph = static_cast<int>(std::round(paperH));
    if (pw < 20 || ph < 20) return;

    // Soft paper drop shadow
    Gdiplus::SolidBrush shBrush(Gdiplus::Color(55, 0, 0, 0));
    g.FillRectangle(&shBrush, px + 4, py + 5, pw, ph);

    int low = job.startNum <= job.endNum ? job.startNum : job.endNum;
    int high = job.startNum >= job.endNum ? job.startNum : job.endNum;
    int totalItems = (high - low) + 1;
    if (totalItems < 1) totalItems = 1;

    int cols = 2, rows = 4;
    GetPrintLayoutGrid(job.layoutIndex, cols, rows);
    int perPage = cols * rows;
    int totalPages = (totalItems + perPage - 1) / perPage;
    if (pageIndex < 0) pageIndex = 0;
    if (pageIndex >= totalPages) pageIndex = totalPages - 1;

    // Render A4 page at crisp preview resolution (1050 x 1485) and downsample smoothly
    Gdiplus::Bitmap a4Bmp(1050, 1485, PixelFormat24bppRGB);
    {
        Gdiplus::Graphics pg(&a4Bmp);
        DrawA4Page(pg, 1050, 1485, style, job, pageIndex * perPage, totalItems);
    }

    g.DrawImage(&a4Bmp, px, py, pw, ph);

    Gdiplus::Pen borderPen(Gdiplus::Color(150, 140, 148, 160), 1.0f);
    g.DrawRectangle(&borderPen, px, py, pw, ph);

    Gdiplus::Graphics outG(hdc);
    outG.DrawImage(&canvas, static_cast<INT>(rc.left), static_cast<INT>(rc.top));
}

bool ExportLabelsToPdf(const std::wstring& pdfPath,
                       const OverlayConfig& style,
                       const PrintJobConfig& job,
                       std::wstring& outError) {
    int low = job.startNum <= job.endNum ? job.startNum : job.endNum;
    int high = job.startNum >= job.endNum ? job.startNum : job.endNum;
    int totalItems = (high - low) + 1;
    if (totalItems <= 0 || totalItems > 1000) {
        outError = L"S\u1ED1 l\u01B0\u1EE3ng nh\u00E3n kh\u00F4ng h\u1EE3p l\u1EC7 (t\u1ED1i \u0111a 1000 nh\u00E3n).";
        return false;
    }

    CLSID jpegClsid = {};
    if (!GetJpegEncoderClsid(jpegClsid)) {
        outError = L"Kh\u00F4ng t\u00ECm th\u1EA5y b\u1ED9 m\u00E3 h\u00F3a JPEG c\u1EE7a Windows GDI+.";
        return false;
    }

    int cols = 2, rows = 4;
    GetPrintLayoutGrid(job.layoutIndex, cols, rows);
    int perPage = cols * rows;
    int totalPages = (totalItems + perPage - 1) / perPage;

    // 250 DPI A4 (210mm x 297mm) = 2067 x 2923 px — razor-sharp print quality for logos & text
    constexpr int kPageW = 2067;
    constexpr int kPageH = 2923;

    std::vector<std::vector<BYTE>> jpegPages;
    jpegPages.reserve(static_cast<size_t>(totalPages));

    for (int p = 0; p < totalPages; ++p) {
        Gdiplus::Bitmap pageBmp(kPageW, kPageH, PixelFormat24bppRGB);
        if (pageBmp.GetLastStatus() != Gdiplus::Ok) {
            outError = L"Kh\u00F4ng \u0111\u1EE7 b\u1ED9 nh\u1EDB \u0111\u1EC3 t\u1EA1o trang in.";
            return false;
        }
        Gdiplus::Graphics g(&pageBmp);
        DrawA4Page(g, kPageW, kPageH, style, job, p * perPage, totalItems);

        std::vector<BYTE> jpgBytes;
        if (!EncodeBitmapToJpeg(pageBmp, jpegClsid, jpgBytes)) {
            outError = L"L\u1ED7i n\u00E9n \u1EA3nh trang PDF.";
            return false;
        }
        jpegPages.push_back(std::move(jpgBytes));
    }

    if (!WriteJpegPagesToPdf(pdfPath, jpegPages, kPageW, kPageH)) {
        outError = L"Kh\u00F4ng th\u1EC3 ghi file PDF (ki\u1EC3m tra file c\u00F3 \u0111ang m\u1EDF b\u1EDFi \u1EE9ng d\u1EE5ng kh\u00E1c kh\u00F4ng).";
        return false;
    }

    return true;
}

bool PrintLabelsDirectly(HWND hwndOwner,
                         const OverlayConfig& style,
                         const PrintJobConfig& job,
                         std::wstring& outError) {
    int low = job.startNum <= job.endNum ? job.startNum : job.endNum;
    int high = job.startNum >= job.endNum ? job.startNum : job.endNum;
    int totalItems = (high - low) + 1;
    if (totalItems <= 0 || totalItems > 1000) {
        outError = L"S\u1ED1 l\u01B0\u1EE3ng nh\u00E3n kh\u00F4ng h\u1EE3p l\u1EC7 (t\u1ED1i \u0111a 1000 nh\u00E3n).";
        return false;
    }

    PRINTDLGW pd = {};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hwndOwner;
    pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;
    if (!PrintDlgW(&pd) || !pd.hDC) {
        return false; // User cancelled
    }

    HDC printDC = pd.hDC;
    int pageW = GetDeviceCaps(printDC, HORZRES);
    int pageH = GetDeviceCaps(printDC, VERTRES);
    if (pageW <= 0 || pageH <= 0) {
        DeleteDC(printDC);
        outError = L"Kh\u00F4ng \u0111\u1ECDc \u0111\u01B0\u1EE3c k\u00EDch th\u01B0\u1EDBc trang m\u00E1y in.";
        return false;
    }

    int cols = 2, rows = 4;
    GetPrintLayoutGrid(job.layoutIndex, cols, rows);
    int perPage = cols * rows;
    int totalPages = (totalItems + perPage - 1) / perPage;

    DOCINFOW di = {};
    di.cbSize = sizeof(di);
    di.lpszDocName = L"Overlay Label - Nhan Phong May";

    if (StartDocW(printDC, &di) <= 0) {
        DeleteDC(printDC);
        outError = L"Kh\u00F4ng th\u1EC3 kh\u1EDFi t\u1EA1o l\u1EC7nh in.";
        return false;
    }

    // Render each page at crisp 200 DPI and blit to printer DC
    constexpr int kRenderW = 1654;
    constexpr int kRenderH = 2339;
    bool ok = true;

    for (int p = 0; p < totalPages; ++p) {
        if (StartPage(printDC) <= 0) {
            ok = false;
            break;
        }

        Gdiplus::Bitmap pageBmp(kRenderW, kRenderH, PixelFormat24bppRGB);
        {
            Gdiplus::Graphics g(&pageBmp);
            DrawA4Page(g, kRenderW, kRenderH, style, job, p * perPage, totalItems);
        }

        Gdiplus::Graphics printG(printDC);
        printG.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        printG.DrawImage(&pageBmp, 0, 0, pageW, pageH);

        if (EndPage(printDC) <= 0) {
            ok = false;
            break;
        }
    }

    if (ok) {
        EndDoc(printDC);
    } else {
        AbortDoc(printDC);
        outError = L"L\u1ED7i trong qu\u00E1 tr\u00ECnh g\u1EEDi d\u1EEF li\u1EC7u t\u1EDBi m\u00E1y in.";
    }

    DeleteDC(printDC);
    return ok;
}
