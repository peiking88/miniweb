#!/usr/bin/env python3
"""
C++ 代码分析脚本
用于快速扫描C++代码中的常见问题模式
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Optional

class CPPAnalyzer:
    """C++ 代码分析器"""
    
    def __init__(self):
        # 定义常见问题模式
        self.patterns = [
            # 内存安全
            {
                'name': 'raw_new_array',
                'pattern': r'new\s+\w+\s*\[',
                'severity': 'high',
                'category': 'memory',
                'message': 'Raw new[] — prefer std::vector or std::unique_ptr'
            },
            {
                'name': 'raw_delete_array',
                'pattern': r'delete\s+\w+\s*\[',
                'severity': 'high',
                'category': 'memory',
                'message': 'Raw delete[] — use RAII'
            },
            # 缓冲区溢出
            {
                'name': 'sprintf',
                'pattern': r'sprintf\s*\(',
                'severity': 'high',
                'category': 'security',
                'message': 'sprintf unsafe — use snprintf or std::format (C++20)'
            },
            {
                'name': 'strcpy',
                'pattern': r'strcpy\s*\(',
                'severity': 'high',
                'category': 'security',
                'message': 'strcpy unsafe — use strncpy or std::string'
            },
            {
                'name': 'gets',
                'pattern': r'gets\s*\(',
                'severity': 'critical',
                'category': 'security',
                'message': 'gets is extremely dangerous — use fgets'
            },
            # 线程不安全函数
            {
                'name': 'strtok',
                'pattern': r'\bstrtok\s*\(',
                'severity': 'high',
                'category': 'concurrency',
                'message': 'strtok is not thread-safe — use strtok_r'
            },
            # 命令注入
            {
                'name': 'system',
                'pattern': r'\bsystem\s*\(',
                'severity': 'high',
                'category': 'security',
                'message': 'system() — command injection risk — use fork+exec'
            },
            # 现代C++
            {
                'name': 'auto_ptr',
                'pattern': r'std::auto_ptr',
                'severity': 'medium',
                'category': 'modern-cpp',
                'message': 'auto_ptr deprecated — use std::unique_ptr'
            },
            # C语言编程规范相关
            {
                'name': 'malloc_no_check',
                'pattern': r'malloc\s*\([^)]+\)\s*;',
                'severity': 'high',
                'category': 'c-programming-guide',
                'message': 'malloc without NULL check — 必须检查返回值是否为NULL'
            },
            {
                'name': 'free_no_null',
                'pattern': r'free\s*\([^)]+\)\s*;(?![^;]*=\s*nullptr)',
                'severity': 'medium',
                'category': 'c-programming-guide',
                'message': 'free后未置NULL — 可能产生野指针'
            },
        ]
        
        # 严重性等级映射
        self.severity_map = {
            'critical': '🔴 P0',
            'high': '🟠 P0',
            'medium': '🟡 P1',
            'low': '🟢 P2'
        }
    
    def analyze_file(self, file_path: Path) -> List[Dict]:
        """分析单个文件"""
        issues = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
                
                for i, line in enumerate(lines, 1):
                    # 跳过注释行
                    stripped_line = line.strip()
                    if stripped_line.startswith('//') or stripped_line.startswith('/*'):
                        continue
                    
                    for pattern_info in self.patterns:
                        # 对于free_no_null模式，需要检查下一行是否设置了nullptr
                        if pattern_info['name'] == 'free_no_null':
                            # 检查当前行是否有free
                            if re.search(r'free\s*\([^)]+\)\s*;', line):
                                # 检查当前行或下一行是否设置了nullptr
                                current_line_has_nullptr = 'nullptr' in line
                                next_line_has_nullptr = False
                                if i < len(lines):
                                    next_line_has_nullptr = 'nullptr' in lines[i]
                                
                                if not (current_line_has_nullptr or next_line_has_nullptr):
                                    issues.append({
                                        'file': str(file_path),
                                        'line': i,
                                        'severity': self.severity_map.get(pattern_info['severity'], '🟢 P2'),
                                        'category': pattern_info['category'],
                                        'message': pattern_info['message'],
                                        'code': line.strip()[:100]
                                    })
                        elif re.search(pattern_info['pattern'], line):
                            issues.append({
                                'file': str(file_path),
                                'line': i,
                                'severity': self.severity_map.get(pattern_info['severity'], '🟢 P2'),
                                'category': pattern_info['category'],
                                'message': pattern_info['message'],
                                'code': line.strip()[:100]  # 截取前100个字符
                            })
        except Exception as e:
            print(f"Error analyzing {file_path}: {e}", file=sys.stderr)
        
        return issues
    
    def analyze_directory(self, directory: Path, extensions: Optional[List[str]] = None) -> List[Dict]:
        """分析目录下的所有文件"""
        if extensions is None:
            extensions = ['.cpp', '.c', '.h', '.hpp', '.cc', '.cxx']
        
        issues = []
        
        for ext in extensions:
            for file_path in directory.rglob(f'*{ext}'):
                if file_path.is_file():
                    issues.extend(self.analyze_file(file_path))
        
        return issues
    
    def print_report(self, issues: List[Dict], output_format: str = 'text'):
        """打印分析报告"""
        if not issues:
            print("✅ 未发现明显问题")
            return
        
        # 按严重性排序
        severity_order = {'🔴 P0': 0, '🟠 P0': 1, '🟡 P1': 2, '🟢 P2': 3}
        issues.sort(key=lambda x: severity_order.get(x['severity'], 99))
        
        if output_format == 'markdown':
            self._print_markdown_report(issues)
        else:
            self._print_text_report(issues)
    
    def _print_text_report(self, issues: List[Dict]):
        """打印文本格式报告"""
        print("=" * 80)
        print("C++ 代码分析报告")
        print("=" * 80)
        
        for issue in issues:
            print(f"{issue['severity']} {issue['category']:15} {issue['file']}:{issue['line']}")
            print(f"  {issue['message']}")
            print(f"  {issue['code']}")
            print()
    
    def _print_markdown_report(self, issues: List[Dict]):
        """打印Markdown格式报告"""
        print("## C++ 代码分析报告")
        print()
        print("| 严重性 | 类别 | 文件 | 行号 | 问题描述 |")
        print("|--------|------|------|------|----------|")
        
        for issue in issues:
            print(f"| {issue['severity']} | {issue['category']} | `{issue['file']}` | {issue['line']} | {issue['message']} |")

def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(description='C++ 代码分析工具')
    parser.add_argument('path', help='要分析的文件或目录路径')
    parser.add_argument('--format', choices=['text', 'markdown'], default='text',
                       help='输出格式 (默认: text)')
    parser.add_argument('--ext', nargs='+', default=['.cpp', '.c', '.h', '.hpp'],
                       help='要分析的文件扩展名 (默认: .cpp .c .h .hpp)')
    
    args = parser.parse_args()
    
    analyzer = CPPAnalyzer()
    path = Path(args.path)
    
    if not path.exists():
        print(f"错误: 路径不存在: {path}", file=sys.stderr)
        sys.exit(1)
    
    if path.is_file():
        issues = analyzer.analyze_file(path)
    else:
        issues = analyzer.analyze_directory(path, args.ext)
    
    analyzer.print_report(issues, args.format)
    
    # 返回退出码：如果有P0问题则返回1，否则返回0
    critical_issues = [i for i in issues if i['severity'] in ['🔴 P0', '🟠 P0']]
    if critical_issues:
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()