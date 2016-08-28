#!/usr/bin/python
#-*-coding:utf-8-*-

import os
import os.path

need_astyle_dir = '..\..\src'
astyle_args = '--style=google --delete-empty-lines --suffix=none --indent=spaces=4 --min-conditional-indent=2 --align-pointer=type --align-reference=type --indent-switches --indent-cases --indent-col1-comments --pad-oper --pad-header --unpad-paren --close-templates --convert-tabs --mode=c'

if os.path.exists('.\\astyle.exe') == False:
	os.system ("copy %s %s" % ('.\\astyle', '.\\astyle.exe'))


for parent,dirnames,filenames in os.walk(need_astyle_dir):
	for filename in filenames:
		need_astyle_file = os.path.join(parent,filename)
		if need_astyle_file.endswith(('.cpp','.h','.CPP','.H','.Cpp')):
			#print need_astyle_file
			os.system(".\\astyle.exe %s %s" % (astyle_args,need_astyle_file))