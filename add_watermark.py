import os

file_path = r'C:\Users\Admin\.gemini\antigravity\scratch\PNG-Studio-Native\qml\components\Sidebar.qml'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

watermark = """
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            
            Column {
                anchors.centerIn: parent
                spacing: 2
                
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
            }
        }
"""

content = content.replace('Item {\n            Layout.fillHeight: true\n        }', watermark + '\n        Item {\n            Layout.fillHeight: true\n        }')

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(content)
print('Watermark added.')
