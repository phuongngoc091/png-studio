import os
import re

base_dir = r'C:\Users\Admin\.gemini\antigravity\scratch\PNG-Studio-Native'

# 1. Rename files and directories containing 'lastudio' or 'la-studio'
for root, dirs, files in os.walk(base_dir, topdown=False):
    for name in files:
        if 'lastudio' in name.lower() or 'la-studio' in name.lower():
            new_name = name.replace('lastudio', 'pngstudio').replace('LA-Studio', 'PNG-Studio').replace('la-studio', 'png-studio').replace('LASTUDIO', 'PNGSTUDIO')
            os.rename(os.path.join(root, name), os.path.join(root, new_name))
    for name in dirs:
        if 'lastudio' in name.lower() or 'la-studio' in name.lower() and name != '.git':
            new_name = name.replace('lastudio', 'pngstudio').replace('LA-Studio', 'PNG-Studio').replace('la-studio', 'png-studio').replace('LASTUDIO', 'PNGSTUDIO')
            os.rename(os.path.join(root, name), os.path.join(root, new_name))

# 2. Replace content inside files
for root, dirs, files in os.walk(base_dir):
    if '.git' in root:
        continue
    for file in files:
        if file.endswith(('.cpp', '.h', '.qml', '.json', '.txt', '.md', '.ts', '.cmake', '.bat', '.ps1')):
            filepath = os.path.join(root, file)
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Replace variations
                new_content = content.replace('lastudio', 'pngstudio')
                new_content = new_content.replace('LA-Studio', 'PNG-Studio')
                new_content = new_content.replace('LA Studio', 'PNG Studio')
                new_content = new_content.replace('la-studio', 'png-studio')
                new_content = new_content.replace('LASTUDIO', 'PNGSTUDIO')
                
                if new_content != content:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)
            except Exception:
                pass

print('Deep rebranding complete.')
