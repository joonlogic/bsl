#!/usr/bin/env python3
# encoding: utf-8

from distutils.core import setup, Extension

bslext_module = Extension('bslext', 
		                  define_macros = [('MAJOR_VERSION', '1'),
						                   ('MINOR_VERSION', '0')],
						  include_dirs = ['../', '../../module/'],
						  libraries = ['bsl.sim', 'pcap', 'pthread'],
						  sources = ['bslext.c'])

setup(name='bslext',
      version='0.1.0',
      description='bsl extension module written in C',
	  author = 'Joon Kim',
	  author_email = 'joon@thefrons.com'
      ext_modules=[bslext_module])
