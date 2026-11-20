# Q-Portal PDFiler

Q-Portal PDFiler is the PDF editor of the Q-Portal intranet: an installable Windows application that lets
users edit, annotate, sign, redact, OCR, merge and split PDF documents **entirely on their own computer**,
so nobody needs online PDF services or paid editors.

It is a rebranded and localized build of [PDF4QT](https://github.com/JakubMelka/PDF4QT) by Jakub Melka,
released under the MIT License. All the credit for the PDF engine and the applications goes to the PDF4QT
author and contributors; this repository adds:

- Q-Portal branding (names, icons, installer, About dialog);
- a complete **Italian translation** (`translations/PDF4QT_it.ts`), selected automatically on Italian Windows;
- the Italian Tesseract model built into the installer, so OCR of Italian documents works offline;
- a 64-bit MSI installer with an Italian setup UI (`.github/workflows/QPortalPDFiler.yml`).

## Applications

| Start menu entry | Executable | Purpose |
|---|---|---|
| Q-Portal PDFiler | `Pdf4QtEditor.exe` | Full editor: text and object editing, annotations, forms, signatures, redaction, OCR, encryption, optimization |
| Q-Portal PDFiler - Visualizzatore | `Pdf4QtViewer.exe` | Lightweight viewer |
| Q-Portal PDFiler - Organizza pagine | `Pdf4QtPageMaster.exe` | Merge, split, insert, delete, move, rotate pages, images to PDF |
| Q-Portal PDFiler - Confronta documenti | `Pdf4QtDiff.exe` | Compare two PDF documents |

## Build

Actions > **Q-Portal PDFiler - Windows MSI** > Run workflow. The artifacts are the MSI, a portable ZIP
and `SHA256SUMS.txt`. Pushing a tag `v*` also creates a draft release.

The product version is `QPORTAL_PDFILER_VERSION` in `CMakeLists.txt`; `PDF4QT_VERSION` stays the version of
the upstream engine.

## Silent deployment

```
msiexec /i Q-Portal-PDFiler_<version>_x64.msi /qn /norestart
```

## Updating from upstream

```
git remote add upstream https://github.com/JakubMelka/PDF4QT.git
git fetch upstream
git merge upstream/master
```

After a merge, refresh the Italian strings with `lupdate` (target language `it`) and translate the new
entries of `translations/PDF4QT_it.ts`.

## License

MIT, see [LICENSE](LICENSE). Third-party libraries keep their own licenses (`3rdparty_licenses/`).
