#!/bin/sh

if git rev-parse --verify HEAD >/dev/null 2>&1
then
	against=HEAD
else
	# Initial commit: diff against an empty tree object
	against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
fi

# We should pass only added or modified C/C++ source files to cppcheck.
changed_files=$(git diff-index --cached $against | \
	grep -E '[MA]	.*\.(c|cpp|h)$' | \
	grep -v 'glog' | \
	cut -d'	' -f 2)

astyle_args="--style=google --delete-empty-lines --suffix=none --indent=spaces=4 --min-conditional-indent=2 --align-pointer=type --align-reference=type --indent-switches --indent-cases --indent-col1-comments --pad-oper --pad-header --unpad-paren --close-templates --convert-tabs --mode=c"
ignore_lists="-legal/copyright,-whitespace/line_length,-build/include_what_you_use,-whitespace/braces,-build/include,-whitespace/blank_line,-whitespace/ending_newline,-build/header_guard,-readability/streams,-whitespace/parens,-whitespace/operators,-build/storage_class,-whitespace/labels"

cpp_astyle_lint_Dir="./tools/cpp_astyle_lint"

cp $cpp_astyle_lint_Dir/astyle $cpp_astyle_lint_Dir/astyle.exe

lint_ret=0
if [ -n "$changed_files" ]; then	
	python $cpp_astyle_lint_Dir/cpplint.py --filter=$ignore_lists $changed_files
	lint_ret=$?
	if [ "$lint_ret" != 0 ]; then
		$cpp_astyle_lint_Dir/astyle.exe $astyle_args $changed_files
		echo -e "[提交失败!!!]\n上传的代码中存在一些不规范的地方, 已经自动对相关代码按照规范要求做了格式化操作, 请重新提交!\n若仍然存在不规范的地方, 请手动修改并提交, 直到所有代码都符合规范为止..."
	fi
	exit $lint_ret
fi
