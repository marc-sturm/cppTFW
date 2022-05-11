#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QList>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QSharedPointer>
#include <QProcess>
#include <QDebug>
#include <QMetaObject>
#include <QMetaMethod>
#include "zlib.h"
#include <cmath>
#include "Exceptions.h"
#include "Helper.h"

namespace TFW
{
    //############## helper functions ##################

    inline QByteArray name(QString filename)
    {
        return QFileInfo(filename).fileName().toUtf8();
    }

    inline QByteArray number(int num)
    {
        return QByteArray::number(num);
    }

    inline bool eq(int a, int e)
    {
        return a==e;
    }

    inline bool eq(double a, double e)
    {
        return fabs(a-e)<0.00001;
    }

    inline bool eq(QString a, QString e)
    {
        return a==e;
    }

    inline QByteArray findTestDataFile(QByteArray sourcefile, QByteArray testfile)
    {
        QByteArray path = QFileInfo(sourcefile).path().toLatin1() + "/" + testfile;
        path.replace("\\", "/").replace("//", "/");
        while(path.contains('/') && !QFile::exists(path))
        {
            path = path.mid(path.indexOf('/')+1);
        }
        if (!QFile::exists(path)) THROW(ProgrammingException, "Could not find test file '" + testfile + "'!");
        return  path;
    }

    //################## status variables ####################

    inline bool& skipped()
    {
        static bool skipped = false;
        return skipped;
    }

    inline bool& failed()
    {
        static bool failed = false;
        return failed;
    }

    inline QByteArray& message()
    {
        static QByteArray message;
        return message;
    }

    //############### test execution ##################

    inline QList<QObject*>& testList()
    {
        static QList<QObject*> list;
        return list;
    }

    inline void addTest(QObject* object)
    {
        QList<QObject*>& list = testList();
        foreach (QObject* test, list)
        {
            if (test->objectName() == object->objectName()) return;
        }
        list.append(object);
    }

    inline int run(int argc, char *argv[])
    {
        //create a QCoreApplication to be able to use a event loop (e.g. for XML validation)
        QCoreApplication core_app(argc, argv);

		//parse command line parameters
		QCommandLineParser parser;
		parser.addOption(QCommandLineOption("s", "Test case string filter for test cases.", "s"));
		parser.addOption(QCommandLineOption("l", "Test case list to execute.", "l"));
		parser.addHelpOption();
		parser.process(core_app);
		QByteArray s_filter = parser.value("s").toLatin1();
		QStringList l_filter;
		if (parser.value("l")!="")
		{
			try
			{
				l_filter = Helper::loadTextFile(parser.value("l"), true, '#', true);
			}
			catch(Exception e)
			{
				qDebug() << "Error loading filter list " << parser.value("l");
				qDebug() << e.message();
				return -1;
			}
		}

        //create folder for test output data
        QDir(".").mkdir("out");

        //open output stream
        QFile outstream;
        outstream.open(stdout, QFile::WriteOnly);

        //run tests
        QTime timer;
        int c_failed = 0;
        int c_skipped = 0;
        int c_passed = 0;
        foreach (QObject* test, testList())
        {
            QByteArray test_name = test->objectName().toUtf8();
            const QMetaObject* object = test->metaObject();
            for (int i=0; i<object->methodCount(); ++i)
            {
                QMetaMethod method = object->method(i);

                //filter for private slots that are not interal QT functions
                if (object->indexOfSlot(method.methodSignature())==-1) continue;
                if (method.access()!=QMetaMethod::Private) continue;
                if (method.name().startsWith("_q_")) continue;

				//string filter
                QByteArray test_and_method = test_name + "::" + method.name();
				if (!test_and_method.contains(s_filter))
				{
					continue;
				}

				//test list filter
				if (!l_filter.isEmpty())
				{
					bool found = false;
					foreach(QString filter, l_filter)
					{
						if (test_and_method+"()"==filter || test_and_method==filter)
						{
							found = true;
						}
					}
					if (!found) continue;
				}

                //execute test
                skipped() = false;
                failed() = false;
                message() = "";
                bool invoked = true;
                timer.restart();
                try
                {
                    invoked = object->invokeMethod(test, method.name());
                }
                catch (Exception& e)
                {
                    QByteArray msg;
                    msg += "exception: Exception (cppCORE)\n";
					msg += "location : " + name(e.file()) + ":" + QByteArray::number(e.line()) + "\n";
					msg += "message  : " + e.message().toUtf8() + "\n";
                    message() = msg;
                    failed() = true;
                }
                catch (std::exception& e)
                {
                    QByteArray msg;
                    msg += "exception: std::exception\n";
					msg += "message  : " + QByteArray(e.what()) + "\n";
                    message() = msg;
                    failed() = true;
                }
                catch (...)
                {
                    message() = "unknown exception";
                    failed() = true;
                }
				if (!invoked) THROW(ProgrammingException, "Could not invoke test method " + test_name + "." + method.name());

                //evaluate what happened
                QByteArray result;
                if (failed())
                {
                    result = "FAIL!";
                    ++c_failed;
                }
                else if(skipped())
                {
                    result = "SKIP";
                    ++c_skipped;
                }
                else
                {
                    result = "PASS";
                    ++c_passed;
                }

                //output
				outstream.write(result + "\t" + test_and_method + "()\t" + Helper::elapsedTime(timer, true) + "\n");
                if (!message().isEmpty())
                {
                    QList<QByteArray> parts = message().trimmed().split('\n');
                    foreach(QByteArray part, parts)
                    {
                        outstream.write("  " + part.trimmed() + "\n");
                    }
                }
				outstream.flush();
            }
        }

        outstream.write("\n");
        outstream.write("PASSED : " + QByteArray::number(c_passed).rightJustified(3, ' ') + "\n");
        outstream.write("SKIPPED: " + QByteArray::number(c_skipped).rightJustified(3, ' ') + "\n");
        outstream.write("FAILED : " + QByteArray::number(c_failed).rightJustified(3, ' ') + "\n");
        outstream.close();

        return c_failed;
    }

    ///Executes a tool and returns (1) if the execution was successful (2) the error message if it was not successful
    inline QString executeTool(QString toolname, QString arguments, bool ignore_error_code, QString file, int line)
    {
        if (QFile::exists(toolname)) //Linux
        {
            toolname = "./" + toolname;
        }
        else if (QFile::exists(toolname + ".exe")) //Windows
        {
            toolname = toolname + ".exe";
        }
        else
        {
            return "Tool '" + toolname + "' not found!";
        }

        QProcess process;
        QString log_file = "out/" + QFileInfo(file).baseName() + "_line" + QString::number(line) + ".log";
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setStandardOutputFile(log_file);
        QStringList arg_split = arguments.split(' ');
        for(int i=0; i<arg_split.count(); ++i)
        {
            arg_split[i].replace("%20", " ");
        }
        process.start(toolname, arg_split);
        bool started = process.waitForStarted(-1);
        bool finished = process.waitForFinished(-1);
        int exit_code = process.exitCode();
        if (!started || !finished || (!ignore_error_code && exit_code!=0))
        {
            QByteArray result = "exit code: " + QByteArray::number(exit_code);
            QFile tmp_file(log_file);
            tmp_file.open(QFile::ReadOnly|QFile::Text);
            result += "\ntool output:\n" + tmp_file.readAll().trimmed();
            return result;
        }
        return "";
    }

    /**
     * @brief comareFiles
     * Compares files line by line to check if they are identical, but uses a delta to check numerics
     * @param actual
     * @param expected
     * @param delta How much absolute deviation should be allowed for numeric values. If delta is 0 then it will not be considered.
     * @param delta_is_percentage If 'true', the delta is interpreted as percentage in the range [0-100] instead of as absolute value.
     * @return empty string on success, otherwise return the diff
     */
    inline QString comareFiles(QString actual, QString expected, double delta, bool delta_is_percentage, char separator)
    {
       actual = QFileInfo(actual).absoluteFilePath();
       expected = QFileInfo(expected).absoluteFilePath();

        //open files
        QFile afile(actual);
        if (!afile.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open actual file '" + actual.toLatin1() + " for reading!";
        QFile efile(expected);
        if (!efile.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open expected file '" + expected.toLatin1() + " for reading!";

        //compare lines
        int line_nr = 1;
        QTextStream astream(&afile);
        QTextStream estream(&efile);
        while (!astream.atEnd() && !estream.atEnd())
        {
            QString aline = astream.readLine();
            QString eline = estream.readLine();
            if(aline!=eline)
            {
                //not delta allowed > no numeric comparison
                if (delta == 0.0)
                {
                    return "Differing line "  + QByteArray::number(line_nr) + "\nactual   : " + aline + "\nexpected : " + eline;
                }

                //numeric comparison
                QStringList a_line_items = aline.split(separator);
                QStringList e_line_items = eline.split(separator);
                if (a_line_items.size() != e_line_items.size())
                {
                    return "Differing line "  + QByteArray::number(line_nr) + " (different token count)\nactual   : " + aline + "\nexpected : " + eline;
                }

                for (int i=0; i<a_line_items.size(); ++i)
                {
                    if (a_line_items[i]!=e_line_items[i])
                    {
                        bool a_item_is_numeric;
                        float a_line_value = a_line_items[i].toFloat(&a_item_is_numeric);

                        bool e_item_is_numeric;
                        float e_line_value = e_line_items[i].toFloat(&e_item_is_numeric);

                        if (!a_item_is_numeric || !e_item_is_numeric)
                        {
                            return "Differing line "  + QByteArray::number(line_nr) + " (non-numeric difference)\nactual   : " + aline + "\nexpected : " + eline;
                        }

                        double abs_diff = fabs(a_line_value-e_line_value);
                        if (delta_is_percentage)
                        {
                            double rel_diff = fabs(a_line_value-e_line_value)/e_line_value;
                            if (rel_diff > delta/100.0)
                            {
                                return "Differing numeric value in line "  + QByteArray::number(line_nr) + " (relative difference too big)\nactual   : " + QString::number(a_line_value) + "\nexpected : " + QString::number(e_line_value) + "\ndelta rel: " + QString::number(rel_diff, 'g', 4);
                            }
                        }
                        else
                        {
                            if (abs_diff > delta)
                            {
                                return "Differing numeric value in line "  + QByteArray::number(line_nr) + " (absolute difference too big)\nactual   : " + QString::number(a_line_value) + "\nexpected : " + QString::number(e_line_value)+ "\ndelta abs: " + QString::number(abs_diff, 'g', 4);
                            }
                        }
                    }
                }
            }
            ++line_nr;
        }

        //compare rest (ignore lines containing only whitespaces)
        QString arest = astream.readAll().trimmed();
        if (!arest.isEmpty()) return "Actual file '" + actual + "' contains more data than expected file '" + expected + "': " + arest;
        QString erest = estream.readAll().trimmed();
        if (!erest.isEmpty()) return "Expected file '" + expected + "' contains more data than actual file '" + actual + "': " + erest;

        return "";
    }

    inline QString comareFilesGZ(QString actual, QString expected)
    {
		//init buffer
		QByteArray buffer(1024, ' ');

		//make file names absolute
		actual = QFileInfo(actual).absoluteFilePath();
		expected = QFileInfo(expected).absoluteFilePath();

		//open streams
		gzFile streama = gzopen(actual.toLatin1().data(),"rb");
		if (streama == NULL)  return "Could not open file '" + actual + "' for reading!";
		gzFile streame = gzopen(expected.toLatin1().data(),"rb");
		if (streame == NULL)  return "Could not open file '" + expected + "' for reading!";

		//compare lines
		int line_nr = 1;
		while (!gzeof(streama) && !gzeof(streame))
		{
			gzgets(streama, buffer.data(), 1024);
			QByteArray aline = QByteArray(buffer.data());
			while (aline.endsWith('\n') || aline.endsWith('\0') || aline.endsWith('\r')) aline.chop(1);

			gzgets(streame, buffer.data(), 1024);
			QByteArray eline = QByteArray(buffer.data());
			while (eline.endsWith('\n') || eline.endsWith('\0') || eline.endsWith('\r')) eline.chop(1);

			if (eline!=aline)
			{
				return "Differing line "  + QByteArray::number(line_nr) + "\nactual   : " + aline + "\nexpected : " + eline;
			}
			++line_nr;
		}

		//check if line counts differ
		if (!gzeof(streama)) return "Actual file '" + actual + "' has more lines than expected file '" + expected + "'!";
		if (!gzeof(streame)) return "Actual file '" + actual + "' has less lines than expected file '" + expected + "'!";

		//close streams
		gzclose(streama);
		gzclose(streame);

		return "";
    }

    inline QString removeLinesMatching(QString filename, QRegExp regexp)
    {
        QFile file(filename);
        QList<QByteArray> output;

        //read input
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open file '" + filename + " for reading!";
        while(!file.atEnd())
        {
            QByteArray line = file.readLine();
            if (regexp.indexIn(line)!=-1) continue;
            output.append(line);
        }
        file.close();

        //store output
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return "Could not open file '" + filename + " for writing!";
        foreach(const QByteArray& line, output)
        {
            file.write(line);
        }
        file.close();

        return "";
    }

    /// Helper class to create a test instance and add it to the test list
    template <class T>
    class TestCreator
    {
    public:
        TestCreator(QString name)
        {
            QObject* inst = new T();
            inst->setObjectName(name);
            addTest(inst);
        }
    };

} //namespace

#define TEST_CLASS(className) \
    class className;\
    static TFW::TestCreator<className> t(#className);\
    class className : public QObject

#define SKIP(msg)\
{\
    TFW::skipped() = true;\
	TFW::message() = QByteArray("message  : ") + #msg + "\n"\
	+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
	return;\
}

//##################### simple comparison macros #####################

#define IS_TRUE(stmt)\
    if ((stmt)==false)\
    {\
        TFW::failed() = true;\
        TFW::message() = "IS_TRUE(" + QByteArray(#stmt) + ") failed\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
        return;\
    }

#define IS_FALSE(stmt)\
    if ((stmt)==true)\
    {\
        TFW::failed() = true;\
        TFW::message() = "IS_FALSE(" + QByteArray(#stmt) + ") failed\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
        return;\
    }

#define I_EQUAL(actual, expected)\
    if (actual!=expected)\
    {\
        TFW::failed() = true;\
        TFW::message() = "I_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "actual   : " + QByteArray::number((qlonglong)actual) + "\n"\
					   + "expected : " + QByteArray::number((qlonglong)expected) + "\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
        return;\
    }


#define F_EQUAL(actual, expected)\
    if (fabs(actual-expected)>0.00000001)\
    {\
        TFW::failed() = true;\
        TFW::message() = "F_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
                       + "actual   : " + QByteArray::number(actual) + "\n"\
                       + "expected : " + QByteArray::number(expected) + "\n"\
                       + "max delta: " + QByteArray::number(0.00000001) + "\n"\
                       + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
        return;\
    }

#define F_EQUAL2(actual, expected, delta)\
    if (fabs(actual-expected)>delta)\
    {\
        TFW::failed() = true;\
		TFW::message() = "F_EQUAL2(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
                       + "actual   : " + QByteArray::number(actual) + "\n"\
                       + "expected : " + QByteArray::number(expected) + "\n"\
                       + "max delta: " + QByteArray::number(delta) + "\n"\
                       + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
        return;\
    }

#define S_EQUAL(actual, expected)\
    if (QString(actual)!=QString(expected))\
    {\
        TFW::failed() = true;\
		TFW::message() = "S_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "actual   : " + QString(actual).toLatin1() + "\n"\
					   + "expected : " + QString(expected).toLatin1() + "\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
        return;\
    }

#define X_EQUAL(actual, expected)\
	if (!(actual==expected))\
	{\
		TFW::failed() = true;\
		TFW::message() = "X_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define IS_THROWN(exception_type, command)\
	{\
		bool thrown = false;\
		bool right = false;\
		try{ command; } catch( exception_type e) { thrown=true; right=true; } catch(...) { thrown=true; } \
		if (!thrown)\
		{\
			TFW::failed() = true;\
			TFW::message() = "IS_THROWN(" + QByteArray(#exception_type) + ", " + QByteArray(#command) + ") failed\n"\
						  + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						  + "message  : no exception thrown";\
			return;\
		}\
		if (!right)\
		{\
			TFW::failed() = true;\
			TFW::message() = "IS_THROWN(" + QByteArray(#exception_type) + ", " + QByteArray(#command) + ") failed\n"\
						  + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						  + "message  : exception thrown, but not a '" + QByteArray(#exception_type) + "'";\
			return;\
		}\
	}
//##################### tool execution and file comparison macros #####################

#define EXECUTE(toolname, arguments) \
	{\
		QString tfw_result = TFW::executeTool(toolname, arguments, false, __FILE__, __LINE__);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "EXECUTE(" + QByteArray(#toolname) + ", " + QByteArray(#arguments) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toLatin1();\
			return;\
		}\
	}

#define EXECUTE_FAIL(toolname, arguments) \
	{\
		QString tfw_result = TFW::executeTool(toolname, arguments, true, __FILE__, __LINE__);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "EXECUTE_FAIL(" + QByteArray(#toolname) + ", " + QByteArray(#arguments) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toLatin1();\
			return;\
		}\
	}

#define COMPARE_FILES(actual, expected)\
	{\
        QString tfw_result = TFW::comareFiles(actual, expected, 0.0, true, '\t');\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "COMPARE_FILES(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toLatin1();\
			return;\
		}\
	}

#define COMPARE_FILES_DELTA(actual, expected, delta, delta_is_percentage, separator)\
        {\
                QString tfw_result = TFW::comareFiles(actual, expected, delta, delta_is_percentage, separator);\
                if (tfw_result!="")\
                {\
                        TFW::failed() = true;\
                        TFW::message() = "COMPARE_FILES_DELTA(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ", " + QByteArray(#delta) + ", " + QByteArray(#delta_is_percentage) + ", " + QByteArray(#separator) + ") failed\n"\
                                                   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
                                                   + "message  : " + tfw_result.toLatin1();\
                        return;\
                }\
        }

#define COMPARE_GZ_FILES(actual, expected)\
	{\
		QString tfw_result = TFW::comareFilesGZ(actual, expected);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "COMPARE_GZ_FILES(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toLatin1();\
			return;\
		}\
	}

#define REMOVE_LINES(filename, regexp)\
	{\
		QString tfw_result = TFW::removeLinesMatching(filename, regexp);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "REMOVE_LINES(" + QByteArray(#filename) + ", " + QByteArray(#regexp) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toLatin1();\
			return;\
		}\
	}

#define TESTDATA(filename)\
     TFW::findTestDataFile(__FILE__, filename)

#endif // TESTFRAMEWORK_H
