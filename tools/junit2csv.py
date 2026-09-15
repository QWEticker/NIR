#!/usr/bin/env python3
import sys
import xml.etree.ElementTree as ET
import csv
from pathlib import Path

def short(s, n=400):
    if s is None: return ""
    s = s.strip()
    return s if len(s) <= n else s[:n]+"..."

def junit_to_csv(xml_path, csv_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()
    rows = []
    if root.tag == 'testsuites':
        suites = root.findall('testsuite')
    elif root.tag == 'testsuite':
        suites = [root]
    else:
        suites = root.findall('.//testsuite')
    for ts in suites:
        suite_name = ts.attrib.get('name','')
        for tc in ts.findall('testcase'):
            name = tc.attrib.get('name','')
            time = tc.attrib.get('time','')
            status = 'PASSED'
            msg = ''
            if tc.find('failure') is not None:
                status = 'FAILED'
                msg = tc.find('failure').text
            elif tc.find('error') is not None:
                status = 'FAILED'
                msg = tc.find('error').text
            elif tc.find('skipped') is not None:
                status = 'SKIPPED'
                msg = tc.find('skipped').text
            sout = ''
            so = tc.find('system-out')
            if so is None:
                so = ts.find('system-out')
            if so is not None and so.text:
                sout = so.text
            rows.append((suite_name, name, status, time, short(msg), short(sout)))
    Path(csv_path).parent.mkdir(parents=True, exist_ok=True)
    with open(csv_path, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['suite','test','status','time_s','message','stdout_preview'])
        w.writerows(rows)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: junit2csv.py input.xml output.csv")
        sys.exit(2)
    junit_to_csv(sys.argv[1], sys.argv[2])