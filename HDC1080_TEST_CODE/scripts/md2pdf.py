#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
md2pdf.py — 마크다운 API 문서를 PDF로 변환한다.

사용법:  python3 scripts/md2pdf.py <input.md> <output.pdf>

- 표/코드펜스를 지원하는 markdown 확장으로 HTML 변환 후 weasyprint로 렌더.
- 한글 표시를 위해 NanumGothic TTF를 scripts/fonts/에 자동 확보(없으면 다운로드)
  하여 @font-face로 임베드한다(시스템 폰트 설치·sudo 불필요).
"""
import os
import shutil
import subprocess
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_DIR = os.path.join(HERE, "fonts")
USER_FONT_DIR = os.path.join(
    os.path.expanduser("~"), ".local", "share", "fonts"
)

FONTS = {
    "NanumGothic-Regular.ttf":
        "https://github.com/google/fonts/raw/main/ofl/nanumgothic/NanumGothic-Regular.ttf",
    "NanumGothic-Bold.ttf":
        "https://github.com/google/fonts/raw/main/ofl/nanumgothic/NanumGothic-Bold.ttf",
}
FONT_FAMILY = "NanumGothic"


def ensure_fonts():
    """NanumGothic TTF를 내려받아 사용자 fontconfig에 등록한다.

    weasyprint가 @font-face(url) 폰트를 서브셋할 때 한글 cmap이 깨지는 문제가
    있어, 폰트를 fontconfig(~/.local/share/fonts)에 설치하고 폰트명으로 참조한다
    (sudo 불필요).
    """
    os.makedirs(FONT_DIR, exist_ok=True)
    os.makedirs(USER_FONT_DIR, exist_ok=True)

    installed = False
    for name, url in FONTS.items():
        cached = os.path.join(FONT_DIR, name)
        if not os.path.exists(cached) or os.path.getsize(cached) == 0:
            print(f"[md2pdf] downloading {name} ...")
            req = urllib.request.Request(url, headers={"User-Agent": "md2pdf"})
            with urllib.request.urlopen(req, timeout=30) as r:
                data = r.read()
            with open(cached, "wb") as f:
                f.write(data)

        target = os.path.join(USER_FONT_DIR, name)
        if not os.path.exists(target) or (
            os.path.getsize(target) != os.path.getsize(cached)
        ):
            shutil.copyfile(cached, target)
            installed = True

    if installed:
        try:
            subprocess.run(
                ["fc-cache", "-f", USER_FONT_DIR],
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        except FileNotFoundError:
            print("[md2pdf] warning: fc-cache 없음 — 한글이 깨질 수 있음")


def build_css():
    return f"""
@page {{
    size: A4;
    margin: 20mm 18mm;
    @bottom-center {{
        content: counter(page) " / " counter(pages);
        font-family: '{FONT_FAMILY}', sans-serif;
        font-size: 9pt;
        color: #888;
    }}
}}
* {{ font-family: '{FONT_FAMILY}', sans-serif; }}
body {{
    font-size: 10.5pt;
    line-height: 1.55;
    color: #1a1a1a;
}}
h1 {{ font-size: 20pt; border-bottom: 3px solid #2a5db0; padding-bottom: 6px; }}
h2 {{ font-size: 15pt; margin-top: 22px; border-bottom: 1px solid #ccc; padding-bottom: 4px; }}
h3 {{ font-size: 12.5pt; margin-top: 16px; color: #2a5db0; }}
table {{
    border-collapse: collapse;
    width: 100%;
    margin: 10px 0;
    font-size: 9.5pt;
}}
th, td {{
    border: 1px solid #b8b8b8;
    padding: 5px 8px;
    text-align: left;
    vertical-align: top;
}}
th {{ background: #eef3fb; font-weight: bold; }}
tr:nth-child(even) td {{ background: #f7f9fc; }}
code {{
    font-family: 'Ubuntu Mono', '{FONT_FAMILY}', monospace;
    background: #f0f0f0;
    padding: 1px 4px;
    border-radius: 3px;
    font-size: 9.5pt;
}}
pre {{
    background: #f6f8fa;
    border: 1px solid #ddd;
    border-radius: 5px;
    padding: 10px 12px;
    overflow-x: auto;
    white-space: pre-wrap;
    word-break: break-word;
}}
pre code {{
    background: none;
    padding: 0;
    font-size: 9pt;
    line-height: 1.4;
}}
hr {{ border: none; border-top: 1px solid #ddd; margin: 18px 0; }}
"""


def main():
    if len(sys.argv) != 3:
        print("usage: python3 scripts/md2pdf.py <input.md> <output.pdf>")
        return 2

    md_path, pdf_path = sys.argv[1], sys.argv[2]

    import markdown  # noqa: E402
    from weasyprint import HTML, CSS  # noqa: E402

    with open(md_path, "r", encoding="utf-8") as f:
        md_text = f.read()

    html_body = markdown.markdown(
        md_text,
        extensions=["tables", "fenced_code", "toc", "sane_lists"],
    )
    html_doc = (
        "<!DOCTYPE html><html><head><meta charset='utf-8'></head>"
        f"<body>{html_body}</body></html>"
    )

    ensure_fonts()
    css = CSS(string=build_css())

    HTML(string=html_doc, base_url=HERE).write_pdf(pdf_path, stylesheets=[css])
    print(f"[md2pdf] wrote {pdf_path} ({os.path.getsize(pdf_path)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
