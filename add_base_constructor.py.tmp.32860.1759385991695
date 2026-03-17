#!/usr/bin/env python3
import re
import sys

# Mapping of class suffixes to their base specialization class names
BASE_CONSTRUCTOR_MAP = {
    'DeathKnightRefactored': 'DeathKnightSpecialization',
    'MageRefactored': 'MageSpecialization',
    'PriestRefactored': 'PriestSpecialization',
    'RogueRefactored': 'RogueSpecialization',
    'ShamanRefactored': 'ShamanSpecialization',
    'WarlockRefactored': 'WarlockSpecialization',
    'WarriorRefactored': 'WarriorSpecialization',
    'PaladinRefactored': 'PaladinSpecialization',
    'MonkRefactored': 'MonkSpecialization',
    'DruidRefactored': 'DruidSpecialization',
    'DemonHunterRefactored': 'DemonHunterSpecialization',
    'EvokerRefactored': 'EvokerSpecialization',
    'HunterRefactored': 'HunterSpecialization',
}

def add_base_constructor_init(file_path):
    """Add base specialization constructor initialization."""

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Determine which base class to add based on filename
    base_class = None
    for suffix, base_name in BASE_CONSTRUCTOR_MAP.items():
        if suffix in file_path:
            base_class = base_name
            break

    if not base_class:
        return False

    # Check if base class constructor is already initialized
    if f'{base_class}(bot)' in content:
        print(f"Base constructor already initialized in {file_path}")
        return False

    # Pattern: Find constructor initialization list
    # Match: explicit ClassName(Player* bot)
    #        : TemplateBase<Resource>(bot)
    # Replace with: explicit ClassName(Player* bot)
    #        : TemplateBase<Resource>(bot)
    #        , BaseClass(bot)

    # More complex regex to handle multi-line constructor initialization
    pattern = r'(explicit\s+\w+Refactored\s*\(\s*Player\s*\*\s*bot\s*\)\s*:\s+\w+Specialization<[^>]+>\s*\(\s*bot\s*\))'

    def replacement(match):
        return f'{match.group(1)}\n        , {base_class}(bot)'

    new_content, count = re.subn(pattern, replacement, content, flags=re.MULTILINE | re.DOTALL)

    if count > 0:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)

        print(f"Added {base_class}(bot) initialization to {file_path}")
        return True

    return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python add_base_constructor.py <file1> <file2> ...")
        sys.exit(1)

    files_modified = 0
    for file_path in sys.argv[1:]:
        try:
            if add_base_constructor_init(file_path):
                files_modified += 1
        except Exception as e:
            print(f"Error processing {file_path}: {e}", file=sys.stderr)

    print(f"\nTotal files modified: {files_modified}")
