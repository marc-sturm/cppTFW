#include "TestFramework.h"

class SomeClass
{
public:
	SomeClass(int i)
		: i_(i)
	{
	}

	bool operator==(const SomeClass& rhs)
	{
		return i_==rhs.i_;
	}

private:
	int i_;
};

TEST_CLASS(BasicStatistics_Test)
{
Q_OBJECT
private slots:

	//################### PASS ######################

	void IS_TRUE_passed()
	{
		IS_TRUE(1+1==2)
	}

	void IS_FALSE_passed()
	{
		IS_FALSE(1+1==3)
	}

	void I_EQUAL_pass()
	{
		I_EQUAL(2+2, 4u)
	}

	void S_EQUAL_pass()
	{
		S_EQUAL(QString("bla"), "bla");
	}

	void F_EQUAL_pass()
	{
		F_EQUAL(1.0, 1.0 + 0.000000001);
	}

	void F_EQUAL2_pass()
	{
		F_EQUAL2(1.0, 1.0 + 0.1, 0.11);
	}

	void X_EQUAL_pass()
	{
		X_EQUAL(SomeClass(1), SomeClass(1));
	}

	void IS_THROWN_pass()
	{
		IS_THROWN(FileAccessException, throw FileAccessException("test", "<file>", 1));
	}

	void COMPARE_FILES_pass()
	{
		COMPARE_FILES(TESTDATA("data/in1.txt"), TESTDATA("data/in1.txt"))
	}

	void REMOVE_LINES_pass()
	{
		QFile::remove("out/in3.tmp");
		QFile::copy(TESTDATA("data/in3.txt"), "out/in3.tmp");
        REMOVE_LINES("out/in3.tmp", QRegularExpression("fourth"))
		COMPARE_FILES("out/in3.tmp", TESTDATA("data/in1.txt"))
	}

	void COMPARE_GZ_FILES_pass()
	{
		COMPARE_GZ_FILES(TESTDATA("data/in1.txt.gz"), TESTDATA("data/in1.txt.gz"))
	}

	void sleeping_pass()
	{
		sleep(2);
	}

	//################### SKIP ######################

	void skipping_inside_test()
	{
		SKIP("skipping test")

		S_EQUAL("this assertion is" , "never reached")
	}

	//################### FAIL ######################

	void throwing_exception()
	{
		THROW(ProgrammingException, "throwing test");
	}

	void S_EQUAL_fail()
	{
		S_EQUAL(QByteArray("bla"), "bluff")
	}

	void I_EQUAL_fail()
	{
		I_EQUAL(2+1, 4)
	}

	void F_EQUAL_fail()
	{
		F_EQUAL(1.0, 1.0 + 0.0001)
	}

	void F_EQUAL2_fail()
	{
		F_EQUAL2(1.0, 1.0 + 0.1, 0.09);
	}

	void IS_TRUE_fail()
	{
		IS_TRUE(1+1==3)
	}

	void IS_FALSE_fail()
	{
		IS_FALSE(2+2==4)
	}

	void X_EQUAL_fail()
	{
		X_EQUAL(SomeClass(1), SomeClass(2));
	}

	void COMPARE_FILES_fail()
	{
		COMPARE_FILES(TESTDATA("data/in1.txt"), TESTDATA("data/in2.txt"))
	}

	void REMOVE_LINES_fail()
	{
		QFile::remove("out/in3.tmp");
		QFile::copy(TESTDATA("data/in3.txt"), "out/in3.tmp");
        REMOVE_LINES("out/in3.tmp", QRegularExpression("string not present"))
		COMPARE_FILES("out/in3.tmp", TESTDATA("data/in1.txt"))
	}

	void COMPARE_GZ_FILES_fail()
	{
		COMPARE_GZ_FILES(TESTDATA("data/in1.txt.gz"), TESTDATA("data/in2.txt.gz"))
	}

	void IS_THROWN_fail()
	{
		IS_THROWN(FileAccessException, int bla=1; bla +=1);
	}

	void IS_THROWN_fail2()
	{
		IS_THROWN(FileAccessException, throw ProgrammingException("bla", "<file>", 4711));
	}

};
