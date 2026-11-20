#!/usr/bin/env bash
# Q-Portal PDFiler - web module build
# Builds BentoPDF (AGPL-3.0, https://github.com/alam00000/bentopdf) for offline intranet hosting
# under the IIS folder /PDFiler, with every WASM module, OCR model and font served locally.
#
# Usage: bash web/build-web.sh [bentopdf-commit] [output-dir]
# Requirements: git, node 18+, npm, curl, zip. Runs on Linux.
set -euo pipefail

BENTO_COMMIT="${1:-f1d685bcf607c904f0c6fb385e8048974a2bc8b0}"
OUT="${2:-$PWD/web-build}"
BASE=/PDFiler
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
WORK="$OUT/work"
DEPLOY="$OUT/deploy/PDFiler"

rm -rf "$OUT"
mkdir -p "$WORK/pkgs" "$WORK/wasm" "$DEPLOY"

echo "==> BentoPDF $BENTO_COMMIT"
git clone -q https://github.com/alam00000/bentopdf.git "$WORK/bentopdf"
git -C "$WORK/bentopdf" checkout -q "$BENTO_COMMIT"
cd "$WORK/bentopdf"
HUSKY=0 npm ci --no-audit --no-fund

PYV=$(sed -n 's/.*pymupdf-wasm@\([0-9.]*\).*/\1/p' .env.example)
GSV=$(sed -n 's/.*gs-wasm@\([0-9.]*\).*/\1/p' .env.example)
CPV=$(sed -n 's/.*coherentpdf@\([0-9.]*\).*/\1/p' .env.example)
TV=$(node -p "require('./package-lock.json').packages['node_modules/tesseract.js'].version")
TCV=$(node -p "require('./package-lock.json').packages['node_modules/tesseract.js-core'].version")

echo "==> WASM modules, OCR and fonts (npm registry)"
cd "$WORK/pkgs"
npm pack "@bentopdf/pymupdf-wasm@$PYV" "@bentopdf/gs-wasm@$GSV" "coherentpdf@$CPV" \
         "tesseract.js@$TV" "tesseract.js-core@$TCV" \
         @tesseract.js-data/ita @tesseract.js-data/eng @tesseract.js-data/osd \
         @embedpdf/fonts-latin@1.0.0 @embedpdf/fonts-jp@1.0.0 @embedpdf/fonts-kr@1.0.0 \
         @embedpdf/fonts-sc@1.0.0 @embedpdf/fonts-tc@1.0.0 @embedpdf/fonts-arabic@1.0.0 \
         @embedpdf/fonts-hebrew@1.0.0 --quiet

W="$WORK/wasm"; T="$(mktemp -d)"
mkdir -p "$W/pymupdf" "$W/gs" "$W/cpdf" "$W/ocr/core" "$W/ocr/lang-data" "$W/ocr/fonts" "$W/embedpdf"
tar xzf bentopdf-pymupdf-wasm-*.tgz -C "$W/pymupdf" --strip-components=1
tar xzf bentopdf-gs-wasm-*.tgz -C "$T"; cp -r "$T/package/assets/"* "$W/gs/"; rm -rf "$T/package"
tar xzf coherentpdf-*.tgz -C "$T"; cp -r "$T/package/dist/"* "$W/cpdf/"; rm -rf "$T/package"
tar xzf "tesseract.js-$TV.tgz" -C "$T"; cp "$T/package/dist/worker.min.js" "$W/ocr/worker.min.js"; rm -rf "$T/package"
tar xzf "tesseract.js-core-$TCV.tgz" -C "$W/ocr/core" --strip-components=1
for l in ita eng osd; do
  tar xzf tesseract.js-data-$l-*.tgz -C "$T"; cp "$T/package/4.0.0_best_int/$l.traineddata.gz" "$W/ocr/lang-data/"; rm -rf "$T/package"
done
curl -fsSL -o "$W/ocr/fonts/NotoSans-Regular.ttf" \
  https://raw.githubusercontent.com/googlefonts/noto-fonts/ffebf8c1ee449e544955a7e813c54f9b73848eac/hinted/ttf/NotoSans/NotoSans-Regular.ttf
declare -A KEEP=([latin]=NotoSans-Regular.ttf [jp]=NotoSansJP-Regular.otf [kr]=NotoSansKR-Regular.otf \
                 [sc]=NotoSansHans-Regular.otf [tc]=NotoSansHant-Regular.otf \
                 [arabic]=NotoNaskhArabic-Regular.ttf [hebrew]=NotoSansHebrew-Regular.ttf)
for p in "${!KEEP[@]}"; do
  d="$W/embedpdf/fonts-$p@1.0.0"; mkdir -p "$d/fonts"
  tar xzf embedpdf-fonts-$p-1.0.0.tgz -C "$T"
  cp "$T/package/fonts/${KEEP[$p]}" "$d/fonts/"; cp "$T/package/LICENSE" "$T/package/package.json" "$d/" 2>/dev/null || true
  rm -rf "$T/package"
done

echo "==> Branding"
cd "$WORK/bentopdf"
cp "$REPO/Pdf4QtEditor/app-icon.svg" public/images/qportal-pdfiler.svg
cp "$REPO/Pdf4QtEditor/app-icon.ico" public/favicon.ico

echo "==> Build"
export HUSKY=0 SIMPLE_MODE=true BASE_URL=$BASE/ COMPRESSION_MODE=o DISABLE_GITHUB_STARS=true \
       ALLOW_INDEXING=false SITE_URL=http://localhost \
       VITE_DEFAULT_LANGUAGE=it VITE_BRAND_NAME="Q-Portal PDFiler" VITE_BRAND_LOGO=images/qportal-pdfiler.svg \
       VITE_FOOTER_TEXT="Q-Portal PDFiler - elaborazione locale nel browser, nessun file lascia il PC. Basato su BentoPDF (AGPL-3.0) - github.com/alam00000/bentopdf" \
       VITE_WASM_PYMUPDF_URL=$BASE/wasm/pymupdf/ VITE_WASM_GS_URL=$BASE/wasm/gs/ VITE_WASM_CPDF_URL=$BASE/wasm/cpdf/ \
       VITE_TESSERACT_WORKER_URL=$BASE/wasm/ocr/worker.min.js VITE_TESSERACT_CORE_URL=$BASE/wasm/ocr/core \
       VITE_TESSERACT_LANG_URL=$BASE/wasm/ocr/lang-data VITE_TESSERACT_AVAILABLE_LANGUAGES=ita,eng \
       VITE_OCR_FONT_BASE_URL=$BASE/wasm/ocr/fonts VITE_EMBEDPDF_FONTS_URL=$BASE/wasm/embedpdf
npm run build

echo "==> Deploy folder"
cp -r dist/. "$DEPLOY/"
cp -r "$W" "$DEPLOY/wasm"
rm -f "$DEPLOY/_headers"
# The editor stylesheet imports Open Sans from Google Fonts: removed, the editor uses the system font (no external request)
grep -rl "@import url('https://fonts.googleapis.com/css2?family=Open+Sans" "$DEPLOY/assets" \
  | xargs -r sed -i "s#@import url('https://fonts.googleapis.com/css2?family=Open+Sans:wght@400;600&display=swap');##"
# Extensionless URLs without the IIS URL Rewrite module: every page X.html is also served as X/index.html
cd "$DEPLOY"
find . -name "*.html" -not -path "./assets/*" -not -path "./pdfjs-*" -not -path "./wasm/*" \
       -not -path "./libreoffice-wasm/*" -not -name "index.html" -not -name "404.html" | while read -r f; do
  d="${f%.html}"; mkdir -p "$d"; cp "$f" "$d/index.html"
done
cp "$HERE/web.config" "$DEPLOY/web.config"

cd "$OUT/deploy"
zip -q -r -9 "$OUT/Q-Portal-PDFiler-Web.zip" PDFiler
sha256sum "$OUT/Q-Portal-PDFiler-Web.zip"
echo "Done: $OUT/Q-Portal-PDFiler-Web.zip"
