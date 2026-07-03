import os

file_path = r'C:\Users\Admin\.gemini\antigravity\scratch\PNG-Studio-Native\qml\components\Sidebar.qml'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

watermark = """        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            
            Column {
                anchors.centerIn: parent
                spacing: 3
                
                Text {
                    text: "TikTok: @phuongngoc091"
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "SĐT: 0932 468 218"
                    color: Theme.accentLight
                    font.pixelSize: 12
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "Credit: Custom version from LA Studio"
                    color: Theme.textSecondary
                    font.pixelSize: 9
                    font.italic: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
"""

if 'id: settingsItem' in content:
    content = content.replace('        Item {\n            id: settingsItem', watermark + '        Item {\n            id: settingsItem')
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print('Watermark and credit inserted.')
else:
    print('Anchor not found.')
