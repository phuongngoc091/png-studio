import os

def replace_in_file(filepath, replacements):
    if not os.path.exists(filepath): return
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    for old, new in replacements.items():
        content = content.replace(old, new)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

base_dir = r'C:\Users\Admin\.gemini\antigravity\scratch\PNG-Studio-Native'

# 1. CMakeLists.txt
replace_in_file(os.path.join(base_dir, 'CMakeLists.txt'), {
    'project(LA-Studio': 'project(PNG-Studio',
    'la-studio': 'png-studio',
    'LA Studio': 'PNG Studio'
})

# 2. src/main.cpp
replace_in_file(os.path.join(base_dir, 'src', 'main.cpp'), {
    '"LA Studio"': '"PNG Studio"',
    'la-studio': 'png-studio'
})

# 3. qml/Main.qml
replace_in_file(os.path.join(base_dir, 'qml', 'Main.qml'), {
    'title: qsTr("LA Studio")': 'title: qsTr("PNG Studio")',
    '"LA Studio"': '"PNG Studio"'
})

print('Rebranding complete.')
