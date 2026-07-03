import os

file_path = r'C:\Users\Admin\.gemini\antigravity\scratch\PNG-Studio-Native\qml\components\Sidebar.qml'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

target_str = """                Text {
                    text: "SĐT: 0932 468 218"
                    color: Theme.accentLight
                    font.pixelSize: 12
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }"""

replacement_str = """                Text {
                    text: "SĐT: 0932 468 218"
                    color: Theme.accentLight
                    font.pixelSize: 12
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "Credit: A custom version from LA Studio"
                    color: Theme.textSecondary
                    font.pixelSize: 10
                    font.italic: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }"""

if target_str in content:
    content = content.replace(target_str, replacement_str)
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print('Credit added.')
else:
    print('Target string not found.')
