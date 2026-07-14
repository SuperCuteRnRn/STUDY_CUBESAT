# -*- coding: utf-8 -*-
"""마크다운 문서 → PDF 변환기 (한글 Malgun Gothic + 표/코드 지원).
   xhtml2pdf(pisa) + python-markdown 사용. 외부 네트워크 불필요."""
import sys
import markdown
from xhtml2pdf import pisa
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from xhtml2pdf.default import DEFAULT_FONT

FONT = "C:/Windows/Fonts/malgun.ttf"

# @font-face 임시복사 버그 회피 — reportlab에 직접 등록 후 xhtml2pdf에 매핑
pdfmetrics.registerFont(TTFont("malgun", FONT))
DEFAULT_FONT["malgun"] = "malgun"

CSS = """
@page { size: A4; margin: 1.8cm 1.6cm; }
body { font-family: malgun; font-size: 9.5pt; line-height: 1.45; color: #1a1a1a; }
h1 { font-size: 18pt; border-bottom: 2px solid #333; padding-bottom: 4px; margin-top: 0; }
h2 { font-size: 13.5pt; border-bottom: 1px solid #bbb; padding-bottom: 2px; margin-top: 16px; }
h3 { font-size: 11.5pt; margin-top: 12px; }
p, li { font-size: 9.5pt; }
code { font-family: malgun; background: #f0f0f0; font-size: 9pt; }
pre { font-family: malgun; background: #f5f5f5; border: 1px solid #ddd;
      padding: 6px 8px; font-size: 8.5pt; }
table { border-collapse: collapse; width: 100%; margin: 8px 0; }
th, td { border: 0.6pt solid #999; padding: 3px 5px; font-size: 8.5pt; }
th { background: #e8e8e8; font-weight: bold; }
blockquote { color: #555; border-left: 3px solid #ccc; padding-left: 8px; margin-left: 0; }
"""

def convert(md_path, pdf_path, title):
    with open(md_path, encoding="utf-8") as f:
        text = f.read()
    body = markdown.markdown(
        text, extensions=["tables", "fenced_code", "sane_lists"])
    html = ("<html><head><meta charset='utf-8'><title>%s</title>"
            "<style>%s</style></head><body>%s</body></html>"
            % (title, CSS, body))
    with open(pdf_path, "wb") as out:
        result = pisa.CreatePDF(html, dest=out, encoding="utf-8")
    if result.err:
        print("  [ERROR]", md_path, "->", pdf_path)
        return False
    print("  [OK]", pdf_path)
    return True

if __name__ == "__main__":
    jobs = [
        ("DOC_TXU_API_20260714.md",
         "Release_ITSDL_TXU_20260714/docs/ITSDL_TXU_API_Reference_Rev1.0.pdf",
         "ITSDL_TRXVU API Reference Rev1.0"),
        ("README.md",
         "Release_ITSDL_TXU_20260714/docs/ITSDL_TXU_README.pdf",
         "ITSDL_TRXVU 테스트 코드 README"),
        ("Release_ITSDL_TXU_20260714/docs/ICD_REFERENCE.txt",
         "Release_ITSDL_TXU_20260714/docs/ITSDL_TXU_ICD_Reference.pdf",
         "ITSDL_TRXVU ICD 참조"),
    ]
    ok = all(convert(m, p, t) for m, p, t in jobs)
    sys.exit(0 if ok else 1)
