#!/usr/bin/env python3
# CGI file upload handler
#
# Parses multipart/form-data by hand instead of relying on the standard
# library "cgi" module: cgi.FieldStorage (and cgitb) were deprecated in
# Python 3.11 and removed entirely in 3.13 (PEP 594), so importing them
# crashes on any current interpreter.

import os
import sys
import time

# Read environment
method = os.environ.get("REQUEST_METHOD", "GET")

# The webserv CGI handler chdir()s into this script's own directory before
# exec'ing it, so DOCUMENT_ROOT (often a relative path like ".") no longer
# means anything by the time we get here — resolve the upload directory
# from this script's real absolute location instead.
script_dir = os.path.dirname(os.path.abspath(__file__))
upload_dir = os.path.join(os.path.dirname(script_dir), "www", "upload")

# Ensure upload directory exists
os.makedirs(upload_dir, exist_ok=True)


def parse_multipart(body, content_type):
    """Split a multipart/form-data body into (filename, content) pairs,
    skipping fields that have no filename (i.e. plain form fields)."""
    marker = "boundary="
    idx = content_type.find(marker)
    if idx == -1:
        return []
    boundary = content_type[idx + len(marker):].strip().strip('"')
    delimiter = ("--" + boundary).encode()

    files = []
    parts = body.split(delimiter)
    for part in parts[1:-1]:
        part = part.strip(b"\r\n")
        if not part:
            continue
        header_end = part.find(b"\r\n\r\n")
        if header_end == -1:
            continue
        headers = part[:header_end].decode("latin-1")
        content = part[header_end + 4:]

        filename = None
        for line in headers.split("\r\n"):
            if line.lower().startswith("content-disposition:") and "filename=" in line:
                fn = line.split("filename=", 1)[1].strip()
                fn = fn.split(";")[0].strip().strip('"')
                if fn:
                    filename = fn
        if filename:
            files.append((os.path.basename(filename), content))
    return files


if method == "GET":
    # Show upload form
    print("Content-Type: text/html")
    print("")
    print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI File Upload</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', sans-serif;
            background: #0f0f1a;
            color: #e2e8f0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            padding: 2rem;
        }
        .card {
            background: #1a1a2e;
            border: 1px solid #2d3748;
            border-radius: 12px;
            padding: 2rem;
            width: 100%;
            max-width: 500px;
        }
        h1 { color: #a3b0ff; margin-bottom: 0.25rem; font-size: 1.5rem; }
        p  { color: #718096; margin-bottom: 1.5rem; font-size: 0.9rem; }
        input[type="file"] {
            background: #0f0f1a;
            border: 1px solid #2d3748;
            color: #e2e8f0;
            padding: 0.6rem;
            border-radius: 8px;
            width: 100%;
            margin-bottom: 1rem;
        }
        button {
            background: #7b8cde;
            color: #fff;
            border: none;
            padding: 0.6rem 1.5rem;
            border-radius: 8px;
            cursor: pointer;
            font-size: 0.95rem;
            font-weight: 600;
            width: 100%;
        }
        button:hover { opacity: 0.85; }
        a { color: #7b8cde; text-decoration: none; display: block;
            margin-top: 1rem; text-align: center; font-size: 0.9rem; }
    </style>
</head>
<body>
<div class="card">
    <h1>CGI File Upload</h1>
    <p>Upload a file using this Python CGI handler. Files are saved to the upload directory.</p>
    <form method="POST" action="/cgi-bin/upload.py" enctype="multipart/form-data">
        <input type="file" name="file" required>
        <button type="submit">Upload File</button>
    </form>
    <a href="/">&#8592; Back to Home</a>
    <a href="/upload/">Browse Uploads</a>
</div>
</body>
</html>""")

elif method == "POST":
    content_type = os.environ.get("CONTENT_TYPE", "")

    uploaded_files = []
    error_msg = ""

    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
        body = sys.stdin.buffer.read(content_length) if content_length > 0 else b""

        if "multipart/form-data" in content_type:
            for filename, content in parse_multipart(body, content_type):
                if not filename:
                    filename = "uploaded_file"
                filename = filename.replace("/", "_").replace("\\", "_")
                save_path = os.path.join(upload_dir, filename)
                with open(save_path, "wb") as f:
                    f.write(content)
                uploaded_files.append(filename)
        elif body:
            # Raw body upload
            filename = "upload_cgi_{}".format(int(time.time()))
            save_path = os.path.join(upload_dir, filename)
            with open(save_path, "wb") as f:
                f.write(body)
            uploaded_files.append(filename)
    except Exception as e:
        error_msg = str(e)

    print("Content-Type: text/html")
    print("")

    if error_msg:
        status_color = "#fc8181"
        status_icon = "&#10060;"
        status_title = "Upload Failed"
        status_detail = "Error: " + error_msg
    elif uploaded_files:
        status_color = "#68d391"
        status_icon = "&#10003;"
        status_title = "Upload Successful"
        status_detail = "Uploaded: " + ", ".join(uploaded_files)
    else:
        status_color = "#f6ad55"
        status_icon = "&#9888;"
        status_title = "No Files Uploaded"
        status_detail = "No file was found in the request."

    print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Upload Result</title>
    <style>
        * {{ box-sizing: border-box; margin: 0; padding: 0; }}
        body {{
            font-family: 'Segoe UI', sans-serif;
            background: #0f0f1a;
            color: #e2e8f0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
        }}
        .card {{
            background: #1a1a2e;
            border: 1px solid #2d3748;
            border-radius: 12px;
            padding: 2rem;
            text-align: center;
            max-width: 450px;
            width: 90%;
        }}
        .icon {{ font-size: 3rem; color: {color}; margin-bottom: 1rem; }}
        h1 {{ font-size: 1.5rem; color: {color}; margin-bottom: 0.5rem; }}
        p  {{ color: #a0aec0; margin-bottom: 1.5rem; font-size: 0.9rem; }}
        a  {{ color: #7b8cde; text-decoration: none; margin: 0.4rem;
              padding: 0.5rem 1rem; border: 1px solid #7b8cde;
              border-radius: 6px; display: inline-block; }}
        a:hover {{ background: #7b8cde; color: #fff; }}
    </style>
</head>
<body>
<div class="card">
    <div class="icon">{icon}</div>
    <h1>{title}</h1>
    <p>{detail}</p>
    <a href="/">&#8592; Home</a>
    <a href="/upload/">Browse Uploads</a>
    <a href="/cgi-bin/upload.py">Upload More</a>
</div>
</body>
</html>""".format(
        color=status_color,
        icon=status_icon,
        title=status_title,
        detail=status_detail
    ))
else:
    print("Content-Type: text/plain")
    print("Status: 405 Method Not Allowed")
    print("")
    print("Method Not Allowed")
