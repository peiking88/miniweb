#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Convert Markdown to PDF using WeasyPrint."""

import markdown
from weasyprint import HTML, CSS
import sys
import os

def create_pdf(md_file, pdf_file):
    """Create PDF from Markdown file using WeasyPrint."""
    # Read markdown content
    with open(md_file, 'r', encoding='utf-8') as f:
        md_content = f.read()
    
    # Convert markdown to HTML
    html_content = markdown.markdown(
        md_content,
        extensions=[
            'tables',
            'fenced_code',
            'toc',
            'nl2br',
            'sane_lists'
        ]
    )
    
    # Create complete HTML document with styling
    full_html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        @page {{
            size: A4;
            margin: 2cm;
        }}
        
        body {{
            font-family: 'Noto Sans CJK SC', 'WenQuanYi Micro Hei', 'DejaVu Sans', Arial, sans-serif;
            font-size: 11pt;
            line-height: 1.6;
            color: #333;
        }}
        
        h1 {{
            font-size: 24pt;
            color: #1a1a1a;
            border-bottom: 3px solid #4a90e2;
            padding-bottom: 10px;
            margin-top: 30px;
            margin-bottom: 20px;
            page-break-before: always;
        }}
        
        h1:first-of-type {{
            page-break-before: avoid;
        }}
        
        h2 {{
            font-size: 18pt;
            color: #2c3e50;
            border-bottom: 2px solid #95a5a6;
            padding-bottom: 8px;
            margin-top: 25px;
            margin-bottom: 15px;
        }}
        
        h3 {{
            font-size: 14pt;
            color: #34495e;
            margin-top: 20px;
            margin-bottom: 10px;
        }}
        
        h4 {{
            font-size: 12pt;
            color: #34495e;
            margin-top: 15px;
            margin-bottom: 8px;
        }}
        
        p {{
            margin-bottom: 10px;
            text-align: justify;
        }}
        
        code {{
            font-family: 'DejaVu Sans Mono', 'Courier New', monospace;
            background-color: #f4f4f4;
            padding: 2px 5px;
            border-radius: 3px;
            font-size: 9pt;
        }}
        
        pre {{
            background-color: #f8f8f8;
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 12px;
            overflow-x: auto;
            font-size: 9pt;
            line-height: 1.4;
            margin: 15px 0;
        }}
        
        pre code {{
            background-color: transparent;
            padding: 0;
        }}
        
        blockquote {{
            border-left: 4px solid #4a90e2;
            margin: 15px 0;
            padding: 10px 20px;
            background-color: #f9f9f9;
            color: #555;
        }}
        
        blockquote p {{
            margin-bottom: 0;
        }}
        
        ul, ol {{
            margin: 10px 0;
            padding-left: 30px;
        }}
        
        li {{
            margin-bottom: 5px;
        }}
        
        a {{
            color: #4a90e2;
            text-decoration: none;
        }}
        
        a:hover {{
            text-decoration: underline;
        }}
        
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 15px 0;
        }}
        
        th, td {{
            border: 1px solid #ddd;
            padding: 8px;
            text-align: left;
        }}
        
        th {{
            background-color: #4a90e2;
            color: white;
            font-weight: bold;
        }}
        
        tr:nth-child(even) {{
            background-color: #f9f9f9;
        }}
        
        hr {{
            border: none;
            border-top: 2px solid #ddd;
            margin: 30px 0;
        }}
        
        strong {{
            font-weight: bold;
            color: #2c3e50;
        }}
        
        em {{
            font-style: italic;
        }}
    </style>
</head>
<body>
{html_content}
</body>
</html>"""
    
    # Convert HTML to PDF
    HTML(string=full_html).write_pdf(pdf_file)
    print(f"PDF created successfully: {pdf_file}")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 convert_to_pdf.py <input.md> [output.pdf]")
        sys.exit(1)
    
    md_file = sys.argv[1]
    
    if len(sys.argv) >= 3:
        pdf_file = sys.argv[2]
    else:
        # Generate output filename from input
        pdf_file = os.path.splitext(md_file)[0] + '.pdf'
    
    if not os.path.exists(md_file):
        print(f"Error: File not found: {md_file}")
        sys.exit(1)
    
    create_pdf(md_file, pdf_file)
