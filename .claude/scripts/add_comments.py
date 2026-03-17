#!C:\Program Files\Python313\python.exe
"""
Auto-comment insertion script for PlayerBot project
Adds comments to undocumented code sections
"""

import os
import re
import json
import shutil
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class CommentInserter:
    """Inserts comments into C++ source files"""
    
    def __init__(self, backup_dir: str = None):
        self.backup_dir = Path(backup_dir) if backup_dir else Path('.claude/backups')
        self.backup_dir.mkdir(parents=True, exist_ok=True)
        
        # Comment templates
        self.function_template = """/**
 * @brief {description}
 * {param_docs}
 * @return {return_doc}
 */"""
        
        self.class_template = """/**
 * @class {class_name}
 * @brief {description}
 * 
 * {detailed_description}
 */"""
        
        self.inline_templates = {
            'null_check': '// Check for null pointer before accessing',
            'loop_start': '// Iterate through {container}',
            'error_check': '// Handle potential error condition',
            'resource_cleanup': '// Clean up allocated resources',
            'todo': '// TODO: Implement {feature}',
            'optimization': '// OPTIMIZE: Consider caching this value',
            'thread_safe': '// THREAD-SAFE: Protected by mutex',
            'hot_path': '// HOT PATH: Performance critical section'
        }
    
    def backup_file(self, file_path: str) -> str:
        """Create backup of file before modification"""
        file_path = Path(file_path)
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        backup_path = self.backup_dir / f"{file_path.name}.{timestamp}.bak"
        
        shutil.copy2(file_path, backup_path)
        logger.info(f"Backup created: {backup_path}")
        return str(backup_path)
    
    def add_function_comment(self, file_path: str, function_data: Dict) -> bool:
        """Add comment to a function"""
        try:
            # Read file
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            # Find function line
            line_num = function_data['line_number'] - 1  # Convert to 0-based
            
            # Check if already has comment
            if line_num > 0 and '/**' in lines[line_num - 1]:
                logger.info(f"Function {function_data['name']} already documented")
                return False
            
            # Build parameter documentation
            param_docs = []
            for param in function_data.get('parameters', []):
                param_docs.append(f" * @param {param['name']} {param['description']}")
            param_doc_str = '\n'.join(param_docs) if param_docs else ' * @param None'
            
            # Build return documentation
            return_doc = 'void' if function_data['return_type'] == 'void' else f"{function_data['return_type']} value"
            
            # Create comment
            comment = self.function_template.format(
                description=function_data['description'],
                param_docs=param_doc_str,
                return_doc=return_doc
            )
            
            # Insert comment before function
            comment_lines = [line + '\n' for line in comment.split('\n')]
            for i, comment_line in enumerate(comment_lines):
                lines.insert(line_num + i, comment_line)
            
            # Write back
            with open(file_path, 'w', encoding='utf-8') as f:
                f.writelines(lines)
            
            logger.info(f"Added comment to {function_data['name']} in {file_path}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to add comment: {e}")
            return False
    
    def add_inline_comments(self, file_path: str, analysis_data: Dict) -> int:
        """Add inline comments based on code analysis"""
        comments_added = 0
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            modified = False
            
            for i, line in enumerate(lines):
                stripped = line.strip()
                
                # Skip if already has comment
                if '//' in line:
                    continue
                
                # Null pointer checks
                if re.match(r'if\s*\(\s*!\s*\w+\s*\)', stripped):
                    if i > 0 and '//' not in lines[i-1]:
                        indent = len(line) - len(line.lstrip())
                        lines[i] = ' ' * indent + '// Null pointer check\n' + lines[i]
                        comments_added += 1
                        modified = True
                
                # For loops
                elif re.match(r'for\s*\(', stripped):
                    if i > 0 and '//' not in lines[i-1]:
                        # Try to extract what we're iterating
                        container_match = re.search(r':\s*(\w+)', stripped)
                        container = container_match.group(1) if container_match else 'collection'
                        indent = len(line) - len(line.lstrip())
                        comment = f"// Iterate through {container}\n"
                        lines[i] = ' ' * indent + comment + lines[i]
                        comments_added += 1
                        modified = True
                
                # Resource cleanup (delete/free)
                elif re.match(r'delete\s+\w+;|free\s*\(', stripped):
                    if i > 0 and '//' not in lines[i-1]:
                        indent = len(line) - len(line.lstrip())
                        lines[i] = ' ' * indent + '// Clean up allocated memory\n' + lines[i]
                        comments_added += 1
                        modified = True
                
                # Error returns
                elif 'return false;' in stripped or 'return nullptr;' in stripped:
                    if i > 0 and '//' not in lines[i-1]:
                        indent = len(line) - len(line.lstrip())
                        lines[i] = ' ' * indent + '// Return error/null on failure\n' + lines[i]
                        comments_added += 1
                        modified = True
                
                # Magic numbers (common WoW values)
                elif re.search(r'\b(1500|2000|3000|5000|10000)\b', stripped):
                    if '//' not in line:
                        # Add inline comment for common timers
                        if '1500' in stripped:
                            lines[i] = lines[i].rstrip() + '  // Global cooldown (1.5 seconds)\n'
                        elif '2000' in stripped:
                            lines[i] = lines[i].rstrip() + '  // 2 second cast time\n'
                        elif '3000' in stripped:
                            lines[i] = lines[i].rstrip() + '  // 3 second cast time\n'
                        elif '5000' in stripped:
                            lines[i] = lines[i].rstrip() + '  // 5 second rule/timer\n'
                        elif '10000' in stripped:
                            lines[i] = lines[i].rstrip() + '  // 10 second cooldown\n'
                        comments_added += 1
                        modified = True
            
            if modified:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.writelines(lines)
                
                logger.info(f"Added {comments_added} inline comments to {file_path}")
            
        except Exception as e:
            logger.error(f"Failed to add inline comments: {e}")
        
        return comments_added
    
    def add_class_comment(self, file_path: str, class_data: Dict) -> bool:
        """Add comment to a class definition"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            line_num = class_data['line_number'] - 1
            
            # Check if already documented
            if line_num > 0 and '/**' in lines[line_num - 1]:
                logger.info(f"Class {class_data['name']} already documented")
                return False
            
            # Generate detailed description
            detailed = f"This class manages {class_data['name'].replace('Bot', 'bot').replace('AI', 'AI ')}"
            if class_data.get('base_classes'):
                detailed += f"\nInherits functionality from {', '.join(class_data['base_classes'])}"
            
            # Create comment
            comment = self.class_template.format(
                class_name=class_data['name'],
                description=class_data['description'],
                detailed_description=detailed
            )
            
            # Insert comment
            comment_lines = [line + '\n' for line in comment.split('\n')]
            for i, comment_line in enumerate(comment_lines):
                lines.insert(line_num + i, comment_line)
            
            # Write back
            with open(file_path, 'w', encoding='utf-8') as f:
                f.writelines(lines)
            
            logger.info(f"Added comment to class {class_data['name']}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to add class comment: {e}")
            return False
    
    def process_documentation_data(self, doc_data: Dict, auto_fix: bool = True) -> Dict:
        """Process documentation data and add missing comments"""
        results = {
            'files_processed': 0,
            'functions_documented': 0,
            'classes_documented': 0,
            'inline_comments_added': 0,
            'backups_created': []
        }
        
        for file_data in doc_data.get('files', []):
            file_path = file_data['file']
            
            # Skip if good documentation coverage
            if file_data.get('statistics', {}).get('documentation_coverage', 0) > 70:
                logger.info(f"Skipping {file_path} - already well documented")
                continue
            
            # Create backup
            if auto_fix:
                backup_path = self.backup_file(file_path)
                results['backups_created'].append(backup_path)
            
            # Process classes
            for class_info in file_data.get('classes', []):
                if auto_fix and self.add_class_comment(file_path, class_info):
                    results['classes_documented'] += 1
            
            # Process functions
            for func_info in file_data.get('functions', []):
                if auto_fix and func_info['complexity'] > 5:  # Only document complex functions
                    if self.add_function_comment(file_path, func_info):
                        results['functions_documented'] += 1
            
            # Add inline comments
            if auto_fix:
                inline_count = self.add_inline_comments(file_path, file_data)
                results['inline_comments_added'] += inline_count
            
            results['files_processed'] += 1
        
        return results

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Add comments to PlayerBot code')
    parser.add_argument('--documentation-file',
                       required=True,
                       help='JSON documentation file from generate_documentation.py')
    parser.add_argument('--backup-dir',
                       default='.claude/backups',
                       help='Directory for file backups')
    parser.add_argument('--dry-run',
                       action='store_true',
                       help='Show what would be done without making changes')
    parser.add_argument('--min-complexity',
                       type=int,
                       default=5,
                       help='Minimum complexity for function documentation')
    
    args = parser.parse_args()
    
    # Load documentation data
    with open(args.documentation_file, 'r') as f:
        doc_data = json.load(f)
    
    # Initialize inserter
    inserter = CommentInserter(args.backup_dir)
    
    # Process files
    results = inserter.process_documentation_data(doc_data, not args.dry_run)
    
    # Report results
    print("\n" + "="*60)
    print("Comment Insertion Complete!")
    print("="*60)
    print(f"Files Processed: {results['files_processed']}")
    print(f"Functions Documented: {results['functions_documented']}")
    print(f"Classes Documented: {results['classes_documented']}")
    print(f"Inline Comments Added: {results['inline_comments_added']}")
    
    if results['backups_created']:
        print(f"\nBackups created in: {args.backup_dir}")
        print("To restore: copy backup files over modified files")

if __name__ == '__main__':
    main()
