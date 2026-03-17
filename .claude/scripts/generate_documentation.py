#!C:\Program Files\Python313\python.exe
"""
Automatic Documentation Generator for PlayerBot Project
Scans all C++ files and generates comprehensive documentation
"""

import os
import re
import json
import argparse
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple, Optional
import logging
from dataclasses import dataclass, asdict
import ast

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class FunctionDoc:
    """Represents documentation for a single function"""
    name: str
    signature: str
    return_type: str
    parameters: List[Dict]
    description: str
    complexity: int
    line_number: int
    file_path: str
    todos: List[str] = None
    notes: List[str] = None
    examples: List[str] = None

@dataclass
class ClassDoc:
    """Represents documentation for a class"""
    name: str
    description: str
    base_classes: List[str]
    methods: List[FunctionDoc]
    members: List[Dict]
    file_path: str
    line_number: int

class CppParser:
    """Parser for C++ code files"""
    
    def __init__(self):
        self.function_pattern = re.compile(
            r'^[\s]*(?:virtual\s+)?(?:static\s+)?(?:inline\s+)?'
            r'([\w:]+(?:\s*<[^>]+>)?)\s+'  # return type
            r'([\w:]+)\s*'                   # function name
            r'\(([^)]*)\)',                  # parameters
            re.MULTILINE
        )
        
        self.class_pattern = re.compile(
            r'^\s*(?:class|struct)\s+'
            r'([\w]+)\s*'              # class name
            r'(?::\s*(?:public|private|protected)\s+([\w:,\s]+))?\s*'  # inheritance
            r'\{',                      # opening brace
            re.MULTILINE
        )
        
        self.member_pattern = re.compile(
            r'^\s*(?:public|private|protected):\s*$|'
            r'^\s*([\w:]+(?:\s*<[^>]+>)?)\s+'  # type
            r'([\w]+)\s*(?:\[[^\]]*\])?;',     # name and optional array
            re.MULTILINE
        )
    
    def parse_file(self, file_path: str) -> Dict:
        """Parse a C++ file and extract documentation data"""
        logger.info(f"Parsing: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            logger.error(f"Failed to read {file_path}: {e}")
            return {}
        
        result = {
            'file': file_path,
            'classes': [],
            'functions': [],
            'statistics': self._calculate_statistics(content)
        }
        
        # Parse classes
        for match in self.class_pattern.finditer(content):
            class_name = match.group(1)
            base_classes = match.group(2).split(',') if match.group(2) else []
            
            # Find class end
            start_pos = match.end()
            end_pos = self._find_matching_brace(content, start_pos)
            class_content = content[start_pos:end_pos] if end_pos != -1 else ""
            
            class_doc = ClassDoc(
                name=class_name,
                description=self._generate_class_description(class_name, base_classes),
                base_classes=[b.strip() for b in base_classes],
                methods=self._parse_methods(class_content, file_path),
                members=self._parse_members(class_content),
                file_path=file_path,
                line_number=content[:match.start()].count('\n') + 1
            )
            result['classes'].append(asdict(class_doc))
        
        # Parse standalone functions
        for match in self.function_pattern.finditer(content):
            if not self._is_inside_class(content, match.start()):
                func_doc = self._create_function_doc(match, content, file_path)
                result['functions'].append(asdict(func_doc))
        
        return result
    
    def _find_matching_brace(self, content: str, start_pos: int) -> int:
        """Find the matching closing brace"""
        brace_count = 1
        pos = start_pos
        
        while pos < len(content) and brace_count > 0:
            if content[pos] == '{':
                brace_count += 1
            elif content[pos] == '}':
                brace_count -= 1
            pos += 1
        
        return pos if brace_count == 0 else -1
    
    def _is_inside_class(self, content: str, pos: int) -> bool:
        """Check if position is inside a class definition"""
        # Simplified check - count braces before position
        before_content = content[:pos]
        open_braces = before_content.count('{')
        close_braces = before_content.count('}')
        return open_braces > close_braces
    
    def _parse_methods(self, class_content: str, file_path: str) -> List[FunctionDoc]:
        """Parse methods within a class"""
        methods = []
        for match in self.function_pattern.finditer(class_content):
            func_doc = self._create_function_doc(match, class_content, file_path)
            methods.append(func_doc)
        return methods
    
    def _parse_members(self, class_content: str) -> List[Dict]:
        """Parse member variables within a class"""
        members = []
        current_access = "private"  # Default access for class
        
        for line in class_content.split('\n'):
            # Check for access specifier
            if re.match(r'^\s*(public|private|protected):\s*$', line):
                current_access = re.match(r'^\s*(public|private|protected):', line).group(1)
                continue
            
            # Check for member variable
            member_match = re.match(r'^\s*([\w:]+(?:\s*<[^>]+>)?)\s+([\w]+)\s*(?:\[[^\]]*\])?\s*;', line)
            if member_match:
                members.append({
                    'type': member_match.group(1).strip(),
                    'name': member_match.group(2).strip(),
                    'access': current_access
                })
        
        return members
    
    def _create_function_doc(self, match, content: str, file_path: str) -> FunctionDoc:
        """Create a FunctionDoc object from regex match"""
        return_type = match.group(1).strip()
        func_name = match.group(2).strip()
        params_str = match.group(3).strip()
        
        # Parse parameters
        parameters = []
        if params_str:
            for param in params_str.split(','):
                param = param.strip()
                if param and param != 'void':
                    # Try to split type and name
                    parts = param.rsplit(' ', 1)
                    if len(parts) == 2:
                        param_type, param_name = parts
                        # Remove pointer/reference from name
                        param_name = param_name.lstrip('*&')
                        parameters.append({
                            'type': param_type,
                            'name': param_name,
                            'description': self._generate_param_description(param_name, param_type)
                        })
        
        # Calculate complexity (simplified McCabe complexity)
        func_end = content.find('{', match.end())
        if func_end != -1:
            func_body_end = self._find_matching_brace(content, func_end + 1)
            func_body = content[func_end:func_body_end] if func_body_end != -1 else ""
            complexity = self._calculate_complexity(func_body)
        else:
            complexity = 1
        
        return FunctionDoc(
            name=func_name,
            signature=match.group(0).strip(),
            return_type=return_type,
            parameters=parameters,
            description=self._generate_function_description(func_name, parameters),
            complexity=complexity,
            line_number=content[:match.start()].count('\n') + 1,
            file_path=file_path,
            todos=self._extract_todos(content[match.start():match.end() + 500]),
            notes=[]
        )
    
    def _calculate_complexity(self, code: str) -> int:
        """Calculate cyclomatic complexity of code"""
        complexity = 1
        # Count decision points
        complexity += code.count('if ')
        complexity += code.count('else if ')
        complexity += code.count('for ')
        complexity += code.count('while ')
        complexity += code.count('case ')
        complexity += code.count('catch ')
        complexity += code.count('&&')
        complexity += code.count('||')
        complexity += code.count('?')  # ternary operator
        return complexity
    
    def _calculate_statistics(self, content: str) -> Dict:
        """Calculate code statistics"""
        lines = content.split('\n')
        total_lines = len(lines)
        
        comment_lines = 0
        code_lines = 0
        blank_lines = 0
        
        in_comment_block = False
        for line in lines:
            stripped = line.strip()
            
            if not stripped:
                blank_lines += 1
            elif stripped.startswith('/*'):
                in_comment_block = True
                comment_lines += 1
            elif stripped.startswith('*/'):
                in_comment_block = False
                comment_lines += 1
            elif in_comment_block or stripped.startswith('//'):
                comment_lines += 1
            else:
                code_lines += 1
        
        documentation_coverage = (comment_lines / total_lines * 100) if total_lines > 0 else 0
        
        return {
            'total_lines': total_lines,
            'code_lines': code_lines,
            'comment_lines': comment_lines,
            'blank_lines': blank_lines,
            'documentation_coverage': round(documentation_coverage, 2)
        }
    
    def _extract_todos(self, content: str) -> List[str]:
        """Extract TODO comments from code"""
        todos = []
        for match in re.finditer(r'//\s*TODO[: ](.*)', content, re.IGNORECASE):
            todos.append(match.group(1).strip())
        return todos
    
    def _generate_class_description(self, class_name: str, base_classes: List[str]) -> str:
        """Generate description for a class based on its name and inheritance"""
        desc = f"Class {class_name}"
        
        # Add context based on name patterns
        if 'AI' in class_name:
            desc = f"Artificial Intelligence controller {class_name}"
        elif 'Bot' in class_name:
            desc = f"Bot management class {class_name}"
        elif 'Strategy' in class_name:
            desc = f"Strategy pattern implementation {class_name}"
        elif 'Manager' in class_name or 'Mgr' in class_name:
            desc = f"Manager class for {class_name.replace('Manager', '').replace('Mgr', '')}"
        
        if base_classes:
            desc += f" (inherits from {', '.join(base_classes)})"
        
        return desc
    
    def _generate_function_description(self, func_name: str, parameters: List[Dict]) -> str:
        """Generate description for a function based on its name and parameters"""
        desc = ""
        
        # Common patterns
        if func_name.startswith('Get') or func_name.startswith('get'):
            desc = f"Retrieves {func_name[3:]}"
        elif func_name.startswith('Set') or func_name.startswith('set'):
            desc = f"Sets {func_name[3:]}"
        elif func_name.startswith('Is') or func_name.startswith('is'):
            desc = f"Checks if {func_name[2:]}"
        elif func_name.startswith('Has') or func_name.startswith('has'):
            desc = f"Checks if has {func_name[3:]}"
        elif 'Update' in func_name:
            desc = f"Updates {func_name.replace('Update', '')}"
        elif 'Init' in func_name:
            desc = f"Initializes {func_name.replace('Init', '')}"
        elif 'Create' in func_name:
            desc = f"Creates {func_name.replace('Create', '')}"
        elif 'Delete' in func_name or 'Destroy' in func_name:
            desc = f"Destroys/deletes {func_name.replace('Delete', '').replace('Destroy', '')}"
        elif 'Handle' in func_name:
            desc = f"Handles {func_name.replace('Handle', '')} event/operation"
        elif 'Process' in func_name:
            desc = f"Processes {func_name.replace('Process', '')}"
        elif 'Cast' in func_name:
            desc = "Casts a spell or ability"
        elif 'Attack' in func_name:
            desc = "Performs attack action"
        elif 'Move' in func_name:
            desc = "Handles movement operation"
        else:
            desc = f"Performs {func_name} operation"
        
        return desc
    
    def _generate_param_description(self, param_name: str, param_type: str) -> str:
        """Generate description for a parameter"""
        desc = ""
        
        # Common parameter patterns
        if 'diff' in param_name.lower():
            desc = "Time difference in milliseconds"
        elif 'target' in param_name.lower():
            desc = "Target unit/object"
        elif 'spell' in param_name.lower():
            desc = "Spell ID or spell information"
        elif 'player' in param_name.lower():
            desc = "Player object reference"
        elif 'creature' in param_name.lower():
            desc = "Creature object reference"
        elif 'damage' in param_name.lower():
            desc = "Damage amount"
        elif 'heal' in param_name.lower():
            desc = "Healing amount"
        elif 'pos' in param_name.lower() or 'position' in param_name.lower():
            desc = "Position coordinates"
        elif 'guid' in param_name.lower():
            desc = "Global unique identifier"
        elif 'id' in param_name.lower():
            desc = "Identifier value"
        elif 'flag' in param_name.lower():
            desc = "Flag value(s)"
        elif 'count' in param_name.lower():
            desc = "Number/count of items"
        elif 'index' in param_name.lower():
            desc = "Array/list index"
        elif 'name' in param_name.lower():
            desc = "Name string"
        elif param_type.startswith('bool'):
            desc = "Boolean flag"
        elif param_type.startswith('uint') or param_type.startswith('int'):
            desc = "Integer value"
        elif param_type.startswith('float') or param_type.startswith('double'):
            desc = "Floating point value"
        elif param_type.find('*') != -1:
            desc = f"Pointer to {param_type.replace('*', '').strip()}"
        elif param_type.find('&') != -1:
            desc = f"Reference to {param_type.replace('&', '').strip()}"
        else:
            desc = f"{param_type} parameter"
        
        return desc

class DocumentationGenerator:
    """Main documentation generator orchestrator"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.parser = CppParser()
        self.documentation_db = {}
        
    def scan_project(self, directories: List[str] = None) -> Dict:
        """Scan entire project for documentation"""
        if directories is None:
            directories = ['src/game/playerbot', 'src/game/AI']
        
        all_docs = {
            'project': 'PlayerBot',
            'scan_date': datetime.now().isoformat(),
            'files': [],
            'total_statistics': {
                'total_files': 0,
                'total_classes': 0,
                'total_functions': 0,
                'total_lines': 0,
                'total_documentation_coverage': 0
            }
        }
        
        for directory in directories:
            dir_path = self.project_root / directory
            if not dir_path.exists():
                logger.warning(f"Directory not found: {dir_path}")
                continue
            
            logger.info(f"Scanning directory: {dir_path}")
            
            # Find all C++ files
            cpp_files = list(dir_path.rglob('*.cpp'))
            cpp_files.extend(list(dir_path.rglob('*.h')))
            cpp_files.extend(list(dir_path.rglob('*.hpp')))
            
            for file_path in cpp_files:
                doc_data = self.parser.parse_file(str(file_path))
                if doc_data:
                    all_docs['files'].append(doc_data)
                    
                    # Update statistics
                    all_docs['total_statistics']['total_files'] += 1
                    all_docs['total_statistics']['total_classes'] += len(doc_data.get('classes', []))
                    all_docs['total_statistics']['total_functions'] += len(doc_data.get('functions', []))
                    
                    stats = doc_data.get('statistics', {})
                    all_docs['total_statistics']['total_lines'] += stats.get('total_lines', 0)
        
        # Calculate average documentation coverage
        if all_docs['files']:
            total_coverage = sum(
                f.get('statistics', {}).get('documentation_coverage', 0) 
                for f in all_docs['files']
            )
            all_docs['total_statistics']['total_documentation_coverage'] = \
                round(total_coverage / len(all_docs['files']), 2)
        
        self.documentation_db = all_docs
        return all_docs
    
    def generate_markdown_report(self, output_path: str = None) -> str:
        """Generate comprehensive Markdown documentation"""
        if output_path is None:
            output_path = self.project_root / '.claude' / 'documentation' / 'PLAYERBOT_DOCUMENTATION.md'
        
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        md_content = []
        md_content.append("# PlayerBot Code Documentation\n")
        md_content.append(f"*Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}*\n\n")
        
        # Summary statistics
        stats = self.documentation_db.get('total_statistics', {})
        md_content.append("## Project Statistics\n")
        md_content.append(f"- **Total Files**: {stats.get('total_files', 0)}\n")
        md_content.append(f"- **Total Classes**: {stats.get('total_classes', 0)}\n")
        md_content.append(f"- **Total Functions**: {stats.get('total_functions', 0)}\n")
        md_content.append(f"- **Total Lines**: {stats.get('total_lines', 0):,}\n")
        md_content.append(f"- **Documentation Coverage**: {stats.get('total_documentation_coverage', 0)}%\n\n")
        
        # Table of contents
        md_content.append("## Table of Contents\n")
        for i, file_data in enumerate(self.documentation_db.get('files', [])):
            file_name = Path(file_data['file']).name
            md_content.append(f"{i+1}. [{file_name}](#{file_name.replace('.', '').lower()})\n")
        md_content.append("\n")
        
        # Detailed documentation for each file
        for file_data in self.documentation_db.get('files', []):
            file_path = Path(file_data['file'])
            file_name = file_path.name
            
            md_content.append(f"## {file_name}\n")
            md_content.append(f"**Path**: `{file_path.relative_to(self.project_root)}`\n\n")
            
            # File statistics
            file_stats = file_data.get('statistics', {})
            md_content.append("### Statistics\n")
            md_content.append(f"- Lines of Code: {file_stats.get('code_lines', 0)}\n")
            md_content.append(f"- Comment Lines: {file_stats.get('comment_lines', 0)}\n")
            md_content.append(f"- Documentation Coverage: {file_stats.get('documentation_coverage', 0)}%\n\n")
            
            # Classes
            classes = file_data.get('classes', [])
            if classes:
                md_content.append("### Classes\n")
                for cls in classes:
                    md_content.append(f"#### `{cls['name']}`\n")
                    md_content.append(f"{cls['description']}\n\n")
                    
                    if cls.get('base_classes'):
                        md_content.append(f"**Inherits from**: {', '.join(cls['base_classes'])}\n\n")
                    
                    # Methods
                    if cls.get('methods'):
                        md_content.append("**Methods**:\n")
                        for method in cls['methods']:
                            params = ', '.join([
                                f"{p['type']} {p['name']}" 
                                for p in method.get('parameters', [])
                            ])
                            md_content.append(f"- `{method['return_type']} {method['name']}({params})`\n")
                            md_content.append(f"  - {method['description']}\n")
                            if method['complexity'] > 10:
                                md_content.append(f"  - ⚠️ **High Complexity**: {method['complexity']}\n")
                    
                    # Members
                    if cls.get('members'):
                        md_content.append("\n**Members**:\n")
                        for member in cls['members']:
                            md_content.append(f"- `{member['access']} {member['type']} {member['name']}`\n")
                    
                    md_content.append("\n")
            
            # Standalone functions
            functions = file_data.get('functions', [])
            if functions:
                md_content.append("### Functions\n")
                for func in functions:
                    params = ', '.join([
                        f"{p['type']} {p['name']}" 
                        for p in func.get('parameters', [])
                    ])
                    md_content.append(f"#### `{func['return_type']} {func['name']}({params})`\n")
                    md_content.append(f"{func['description']}\n")
                    
                    if func.get('parameters'):
                        md_content.append("\n**Parameters**:\n")
                        for param in func['parameters']:
                            md_content.append(f"- `{param['name']}`: {param['description']}\n")
                    
                    if func.get('todos'):
                        md_content.append("\n**TODOs**:\n")
                        for todo in func['todos']:
                            md_content.append(f"- {todo}\n")
                    
                    if func['complexity'] > 10:
                        md_content.append(f"\n⚠️ **Complexity Warning**: Cyclomatic complexity is {func['complexity']}\n")
                    
                    md_content.append("\n")
            
            md_content.append("---\n\n")
        
        # Write to file
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(''.join(md_content))
        
        logger.info(f"Documentation written to: {output_path}")
        return str(output_path)
    
    def generate_html_report(self, output_path: str = None) -> str:
        """Generate HTML documentation with styling"""
        if output_path is None:
            output_path = self.project_root / '.claude' / 'documentation' / 'index.html'
        
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        html_content = []
        html_content.append("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PlayerBot Code Documentation</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background: #f5f5f5;
        }
        h1, h2, h3, h4 {
            color: #2c3e50;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 30px;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        .stat-card {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .stat-value {
            font-size: 2em;
            font-weight: bold;
            color: #667eea;
        }
        .stat-label {
            color: #666;
            font-size: 0.9em;
        }
        .file-section {
            background: white;
            margin: 20px 0;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .class-box {
            border-left: 4px solid #667eea;
            padding-left: 15px;
            margin: 15px 0;
        }
        .function-box {
            border-left: 4px solid #764ba2;
            padding-left: 15px;
            margin: 15px 0;
        }
        code {
            background: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Consolas', 'Monaco', monospace;
        }
        .complexity-high {
            color: #e74c3c;
            font-weight: bold;
        }
        .complexity-medium {
            color: #f39c12;
        }
        .coverage-good {
            color: #27ae60;
        }
        .coverage-poor {
            color: #e74c3c;
        }
        .toc {
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 30px;
        }
        .toc ul {
            list-style: none;
            padding-left: 20px;
        }
        .toc a {
            color: #667eea;
            text-decoration: none;
        }
        .toc a:hover {
            text-decoration: underline;
        }
        .method-list {
            margin-left: 20px;
        }
        .param {
            color: #e74c3c;
        }
        .return-type {
            color: #3498db;
        }
    </style>
</head>
<body>
""")
        
        # Header
        html_content.append('<div class="header">')
        html_content.append('<h1>🎮 PlayerBot Code Documentation</h1>')
        html_content.append(f'<p>Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>')
        html_content.append('</div>')
        
        # Statistics
        stats = self.documentation_db.get('total_statistics', {})
        html_content.append('<div class="stats-grid">')
        
        html_content.append('<div class="stat-card">')
        html_content.append(f'<div class="stat-value">{stats.get("total_files", 0)}</div>')
        html_content.append('<div class="stat-label">Total Files</div>')
        html_content.append('</div>')
        
        html_content.append('<div class="stat-card">')
        html_content.append(f'<div class="stat-value">{stats.get("total_classes", 0)}</div>')
        html_content.append('<div class="stat-label">Classes</div>')
        html_content.append('</div>')
        
        html_content.append('<div class="stat-card">')
        html_content.append(f'<div class="stat-value">{stats.get("total_functions", 0)}</div>')
        html_content.append('<div class="stat-label">Functions</div>')
        html_content.append('</div>')
        
        coverage = stats.get('total_documentation_coverage', 0)
        coverage_class = 'coverage-good' if coverage > 50 else 'coverage-poor'
        html_content.append('<div class="stat-card">')
        html_content.append(f'<div class="stat-value {coverage_class}">{coverage}%</div>')
        html_content.append('<div class="stat-label">Documentation Coverage</div>')
        html_content.append('</div>')
        
        html_content.append('</div>')
        
        # Table of contents
        html_content.append('<div class="toc">')
        html_content.append('<h2>📚 Table of Contents</h2>')
        html_content.append('<ul>')
        for file_data in self.documentation_db.get('files', []):
            file_name = Path(file_data['file']).name
            html_content.append(f'<li><a href="#{file_name.replace(".", "_")}">{file_name}</a></li>')
        html_content.append('</ul>')
        html_content.append('</div>')
        
        # File documentation
        for file_data in self.documentation_db.get('files', []):
            file_path = Path(file_data['file'])
            file_name = file_path.name
            
            html_content.append(f'<div class="file-section" id="{file_name.replace(".", "_")}">')
            html_content.append(f'<h2>📄 {file_name}</h2>')
            html_content.append(f'<p><strong>Path:</strong> <code>{file_path.relative_to(self.project_root)}</code></p>')
            
            # Classes
            for cls in file_data.get('classes', []):
                html_content.append('<div class="class-box">')
                html_content.append(f'<h3>Class: {cls["name"]}</h3>')
                html_content.append(f'<p>{cls["description"]}</p>')
                
                if cls.get('methods'):
                    html_content.append('<h4>Methods:</h4>')
                    html_content.append('<div class="method-list">')
                    for method in cls['methods']:
                        complexity_class = ''
                        if method['complexity'] > 15:
                            complexity_class = 'complexity-high'
                        elif method['complexity'] > 10:
                            complexity_class = 'complexity-medium'
                        
                        html_content.append(f'<div>')
                        html_content.append(f'<code><span class="return-type">{method["return_type"]}</span> ')
                        html_content.append(f'{method["name"]}(')
                        
                        params = ', '.join([f'<span class="param">{p["type"]} {p["name"]}</span>' 
                                          for p in method.get('parameters', [])])
                        html_content.append(params)
                        html_content.append(')</code>')
                        
                        if complexity_class:
                            html_content.append(f' <span class="{complexity_class}">[Complexity: {method["complexity"]}]</span>')
                        
                        html_content.append(f'<p style="margin-left: 20px; color: #666;">{method["description"]}</p>')
                        html_content.append('</div>')
                    html_content.append('</div>')
                
                html_content.append('</div>')
            
            # Functions
            for func in file_data.get('functions', []):
                html_content.append('<div class="function-box">')
                html_content.append(f'<h3>Function: {func["name"]}</h3>')
                html_content.append(f'<p>{func["description"]}</p>')
                
                html_content.append('<code>')
                html_content.append(f'<span class="return-type">{func["return_type"]}</span> ')
                html_content.append(f'{func["name"]}(')
                params = ', '.join([f'<span class="param">{p["type"]} {p["name"]}</span>' 
                                  for p in func.get('parameters', [])])
                html_content.append(params)
                html_content.append(')</code>')
                
                if func['complexity'] > 10:
                    complexity_class = 'complexity-high' if func['complexity'] > 15 else 'complexity-medium'
                    html_content.append(f' <span class="{complexity_class}">[Complexity: {func["complexity"]}]</span>')
                
                html_content.append('</div>')
            
            html_content.append('</div>')
        
        html_content.append('</body></html>')
        
        # Write to file
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(''.join(html_content))
        
        logger.info(f"HTML documentation written to: {output_path}")
        return str(output_path)
    
    def generate_json_database(self, output_path: str = None) -> str:
        """Save documentation as JSON database"""
        if output_path is None:
            output_path = self.project_root / '.claude' / 'documentation' / 'documentation.json'
        
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(self.documentation_db, f, indent=2)
        
        logger.info(f"JSON database written to: {output_path}")
        return str(output_path)

def main():
    parser = argparse.ArgumentParser(description='Generate documentation for PlayerBot project')
    parser.add_argument('--project-root', 
                       default='C:/TrinityBots/TrinityCore',
                       help='Project root directory')
    parser.add_argument('--directories',
                       nargs='+',
                       default=['src/game/playerbot'],
                       help='Directories to scan')
    parser.add_argument('--output-format',
                       choices=['markdown', 'html', 'json', 'all'],
                       default='all',
                       help='Output format for documentation')
    parser.add_argument('--output-dir',
                       default=None,
                       help='Output directory for documentation')
    
    args = parser.parse_args()
    
    # Initialize generator
    generator = DocumentationGenerator(args.project_root)
    
    # Scan project
    logger.info("Starting documentation scan...")
    docs = generator.scan_project(args.directories)
    
    # Generate outputs
    outputs = []
    
    if args.output_format in ['markdown', 'all']:
        output_path = generator.generate_markdown_report()
        outputs.append(('Markdown', output_path))
    
    if args.output_format in ['html', 'all']:
        output_path = generator.generate_html_report()
        outputs.append(('HTML', output_path))
    
    if args.output_format in ['json', 'all']:
        output_path = generator.generate_json_database()
        outputs.append(('JSON', output_path))
    
    # Summary
    print("\n" + "="*60)
    print("Documentation Generation Complete!")
    print("="*60)
    
    stats = docs.get('total_statistics', {})
    print(f"\nProject Statistics:")
    print(f"  Files Scanned: {stats.get('total_files', 0)}")
    print(f"  Classes Found: {stats.get('total_classes', 0)}")
    print(f"  Functions Found: {stats.get('total_functions', 0)}")
    print(f"  Total Lines: {stats.get('total_lines', 0):,}")
    print(f"  Documentation Coverage: {stats.get('total_documentation_coverage', 0)}%")
    
    print(f"\nGenerated Outputs:")
    for format_type, path in outputs:
        print(f"  {format_type}: {path}")
    
    # Recommendations
    if stats.get('total_documentation_coverage', 0) < 50:
        print("\n⚠️ Warning: Documentation coverage is below 50%")
        print("   Consider running with --auto-comment flag to add missing documentation")

if __name__ == '__main__':
    main()
