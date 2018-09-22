### 利用git hook实现C++代码提交前自动规范检查&格式化工具


在平时的开发过程中，尤其是多人共同参与开发的项目中，编码规范的重要性无需多说。由于个人工作主要以C++为主，因此，本文介绍的主要是C++编码规范相关。关于C++编程规范网上有很多，对比了一下，个人比较喜欢Google的C++编码规范，cpplint.py就是Google提供的一个C++代码规范检查的python脚本。本文介绍如何利用git hook实现C++代码提交前自动规范检查以及自动进行代码格式化，主要是通过脚本对cpplint.py以及astyle.exe的操作进行了封装，并且通过git hook技术来实现在提交代码前自动执行该脚本。



工具完整代码详见：
[https://github.com/AnonymousRookie/useful-tools/tree/master/cpp_astyle_lint](https://github.com/AnonymousRookie/useful-tools/tree/master/cpp_astyle_lint)


### 程序目录结构

程序目录结构如下：

```
cpp_astyle_lint/
├── astyle
├── astyle_all_src_codes.bat
├── astyle_all_src_codes.py
├── astyle-help.txt
├── cpp_astyle_lint_changed_codes.sh
├── cpplint_all_src_codes.bat
├── cpplint_all_src_codes.py
├── cpplint.py
├── install.bat
└── pre-commit
```

### 使用方法

该工具的使用非常简单，在.git的同级目录中创建tools文件夹，将[cpp_astyle_lint](https://github.com/AnonymousRookie/useful-tools/tree/master/cpp_astyle_lint)拷贝到tools文件夹中，运行install.bat即可将pre-commit拷贝到.git/hooks中。下次提交代码时，会自动对提交的源文件进行规范检查，如果检查不通过，则会自动对该源文件进行格式化，并提示不符合规范的地方，我们可以根据提示进行修改，直到提交的代码符合规范为止才可提交成功。


### 各脚本介绍

接下来分别对上述部分脚本进行简要介绍：


- install.bat

该脚本只是执行简单的拷贝操作:
```
XCOPY .\pre-commit ..\..\.git\hooks /s /e /y
```

- pre-commit

git hook的一种，在执行提交前运行，git hook的路径为.git/hooks，该目录中有很多种git hook sample script，我们可以根据需要进行修改。在本工具中，pre-commit直接调用了cpp_astyle_lint_changed_codes.sh 脚本。


- cpp_astyle_lint_changed_codes.sh

该脚本首先通过cpplint.py对提交的代码进行规范检查，如果检查不通过，则通过astyle.exe自动对该源文件进行了格式化操作(例如：自动在等号两端的添加空格等)。提交失败时，我们可以看到详细提示，根据提示逐条修改即可，直到全都满足规范方可提交成功。

```sh
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
    grep -E '[MA]   .*\.(c|cpp|h)$' | \
    grep -v 'glog' | \
    cut -d' ' -f 2)

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
```


- astyle_all_src_codes.bat

对全部源文件进行格式化。

- cpplint_all_src_codes.bat

对全部源文件进行规范检查。


### 参数设置

在进行代码格式化时，可以根据需要设置一些参数，例如：想自动删除代码中的空行，可以在astyle_args中添加 '--style=google --delete-empty-lines'，更多参数详见astyle-help.txt，本工具使用的参数如下：

```sh
astyle_args = '--style=google --delete-empty-lines --suffix=none --indent=spaces=4 --min-conditional-indent=2 --align-pointer=type --align-reference=type --indent-switches --indent-cases --indent-col1-comments --pad-oper --pad-header --unpad-paren --close-templates --convert-tabs --mode=c'
```


在进行代码规范检查时，如果觉得某条规范并不符合项目中大多数成员的编码习惯，可以修改参数ignore_lists，从而忽略该项的检查。
本工具使用的参数如下：

```sh
ignore_lists="-legal/copyright,-whitespace/line_length,-build/include_what_you_use,-whitespace/braces,-build/include,-whitespace/blank_line,-whitespace/ending_newline,-build/header_guard,-readability/streams,-whitespace/parens,-whitespace/operators,-build/storage_class,-whitespace/labels"
```

### 测试示例

本文编写了一个不符合Google的C++编码规范的程序进行测试：
```c
//just for test
void func(int& x) {
    for (int i=0;i<10;++i) {
        do(i * x);
    }
}
```

当我们进行提交时，将会提示如下信息：
```sh
$ git commit -am "test"
test.cpp:1:  Should have a space between // and comment  [whitespace/comments] [4]
test.cpp:2:  Is this a non-const reference? If so, make const or use a pointer: int& x  [runtime/references] [2]
test.cpp:3:  Missing space after ;  [whitespace/semicolon] [3]
Done processing test.cpp
Total errors found: 3
Formatted  E:\SourceCodes\Test\test.cpp

Cannot convert to multi-byte string, reverting to English

[提交失败!!!]
上传的代码中存在一些不规范的地方, 已经自动对相关代码按照规范要求做了格式化操作, 请重新提交!
若仍然存在不规范的地方, 请手动修改并提交, 直到所有代码都符合规范为止...
```

以上提示中包含有3处错误：
1. 第一行注释后面应该有一个空格；
2. 第二行使用了非常引用，规范建议要么使用const int& x，要么使用int* x;
3. 第三行分号后面缺少空格。


我们再次查看该源文件发现代码已经被格式化：
```c
//just for test
void func(int& x) {
    for (int i = 0; i < 10; ++i) {
        do(i * x);
    }
}
```

再次提交：

```sh
test.cpp:1:  Should have a space between // and comment  [whitespace/comments] [4]
test.cpp:2:  Is this a non-const reference? If so, make const or use a pointer: int& x  [runtime/references] [2]
Done processing test.cpp
Total errors found: 2
Unchanged  E:\SourceCodes\Test\test.cpp

Cannot convert to multi-byte string, reverting to English

[提交失败!!!]
上传的代码中存在一些不规范的地方, 已经自动对相关代码按照规范要求做了格式化操作, 请重新提交!
若仍然存在不规范的地方, 请手动修改并提交, 直到所有代码都符合规范为止...
```

可以发现，通过上一次提交自动进行格式化，本次提交少了一个错误提示。接下来根据提示，逐条修改即可，手动修改如下：

```c
// just for test
void func(const int& x) {
    for (int i = 0; i < 10; ++i) {
        do(i * x);
    }
}
```

再次提交发现没有错误，提交成功。

```sh
$ git commit -am "test"
Done processing test.cpp
[master 496e4c9] test
 2 files changed, 7 insertions(+), 24 deletions(-)
 rewrite test.cpp (98%)
```