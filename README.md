# cppTFW
Simple C++ test framework based on [Qt](http://www.qt.io), but not on _QTest_.  
The framework consists of one header only and needs no compilation.

## Test setup
Class tests are created by deriving a test class from _QObject_ in a header file and defining _private slots_ for each test case.  
__SomeClass\_Test.h:__

	#include "TestFramework.h"
	
	TEST_CLASS(SomeClass_Test)
	{
	Q_OBJECT
	private slots:
	
		void TestCase1()
		{
			I_EQUAL(1+1, 2)
		}

		void TestCase2()
		{
			IS_FALSE(1+1==3)
		}
	}

The execution of all test classes is executed by this simple call:  
__main.cpp:__

	#include "TestFramework.h"
	
	int main(int argc, char *argv[])
	{
		return TFW::run(argc, argv);
	}


## Text executable command-line parameters
The test executable can be invoked with these parameters:

 * `-s` Filter string for test cases. Tests that conain the string (case-sensitive) are executed.
 * `-l` Test case list file. Tests that are contained in the given text file are executed.
 * `-h` Prints a short help.

## Basic comparison macros
 * `IS_TRUE(expression)` Checks that the expression evaluates to _true_.
 * `IS_FALSE(expression)` Checks that the expression evaluates to _false_.
 * `I_EQUAL(actual, expected)` Integer equality check.
 * `F_EQUAL(actual, expected)` Float equality check (with default accuracy).
 * `F_EQUAL2(actual, expected, delta)` Float equality ceck (with custom accuracy).
 * `S_EQUAL(actual, expected)` String equality check.
 * `X_EQUAL(actual, expected)` General equality check.
 * `IS_THROWN(exception, expression)` Checks that the _exception_ type is thrown by the _expression_.	

##File handling macros
 * `EXECUTE(toolname, arguments)` Executes a tool from the same folder and checks the error code.
 * `EXECUTE_FAIL(toolname, arguments)` Executes a tool from the same folder and ignores the error code.
 * `TESTDATA(filename)` Locates test data relative to the test source file.
 * `REMOVE_LINES(filename, regexp)` Removes lines that match the given _QRegExp_ form a file.
 * `COMPARE_FILES(actual, expected)` File equality check.
 * `COMPARE_GZ_FILES(actual, expected)` File equality check for gzipped files.
