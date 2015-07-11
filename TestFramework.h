#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H

#include <QTest>
#include <QList>
#include <QString>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QSharedPointer>
#include <QProcess>
#include <QDebug>
#include "zlib.h"
#include "math.h"
#include "Exceptions.h"

namespace TFW
{
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

            //create folder for test output data
            QDir(".").mkdir("out");

            //run tests
            int ret = 0;
            foreach (QObject* test, testList())
            {
                    try
                    {
                            //run selected tests
                            if (argc==2 && argv[1][0]!='-')
                            {
                                    if (test->objectName().contains(argv[1]))
                                    {
                                        ret += QTest::qExec(test);
                                    }
                            }
                            //run all tests
                            else
                            {
                                ret += QTest::qExec(test, argc, argv);
                            }
                    }
                    catch (Exception& e)
                    {
                            qDebug() << "cppCORE::Exception: " + e.message();
                            ret += 1;
                    }
                    catch (std::exception& e)
                    {
                            qDebug() << "std::exception: " + QString(e.what());
                            ret += 1;
                    }
                    catch (...)
                    {
                            ret += 1;
                    }
            }

            QFile file;
            file.open(stdout, QFile::WriteOnly);
            file.write("######### " + QByteArray::number(ret) + " TEST(S) FAILED #########\n");
            file.close();

            return ret;
    }

    inline void executeTool(QString toolname, QString arguments, bool expect_error, QString file, int line)
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
                    QFAIL(QTest::toString("Execution of tool '" + toolname + "' failed: Tool not present!"));
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
            if (!started || !finished || (!expect_error && exit_code!=0))
            {
                    QString add_info = "\n  Arguments: " + arguments;
                    add_info += "\n  Exit code: " + QString::number(exit_code);
                    QFile tmp_file(log_file);
                    tmp_file.open(QFile::ReadOnly|QFile::Text);
                    add_info += "\n  Output   : " + tmp_file.readAll();
                    QFAIL(QTest::toString("Execution of tool '" + toolname + "' failed. " + add_info));
            }
    }

    inline void comareFiles(QString actual, QString expected)
    {
            //make file names absolute
            actual = QFileInfo(actual).absoluteFilePath();
            expected = QFileInfo(expected).absoluteFilePath();

            //open files
            QFile afile(actual);
            if (!afile.open(QIODevice::ReadOnly | QIODevice::Text)) QFAIL(QTest::toString("Could not open actual file '" + actual + " for reading!"));
            QFile efile(expected);
            if (!efile.open(QIODevice::ReadOnly | QIODevice::Text)) QFAIL(QTest::toString("Could not open expected file '" + expected + " for reading!"));

            //compare lines
            int line_nr = 1;
            QTextStream astream(&afile);
            QString aline = astream.readLine();
            QTextStream estream(&efile);
            QString eline = estream.readLine();
            while (!aline.isNull() && !eline.isNull())
            {
                                    QVERIFY2(aline==eline, QTest::toString("Differing line '"  + QString::number(line_nr) + "' in file '" + QFileInfo(actual).fileName() + "'.\nA:" + aline + "\nE:" + eline));
                    aline = astream.readLine();
                    eline = estream.readLine();
                    ++line_nr;
            }

            if (!aline.isNull()) QFAIL(QTest::toString("Actual file '" + actual + "' has more lines than expected file '" + expected + "'!"));
            if (!eline.isNull()) QFAIL(QTest::toString("Actual file '" + actual + "' has less lines than expected file '" + expected + "'!"));
    }

    inline void compareDouble(double actual, double expected, double delta)
    {
            QVERIFY2(fabs(actual-expected) < delta, QTest::toString("Double comparison failed - actual:" + QString::number(actual) + " expected:"  + QString::number(expected) +  " delta:"  + QString::number(delta)));
    }

    inline void comareFilesGZ(QString actual, QString expected)
    {
            //init buffer
            QByteArray buffer(1024, ' ');

            //make file names absolute
            actual = QFileInfo(actual).absoluteFilePath();
            expected = QFileInfo(expected).absoluteFilePath();

            //open streams
            gzFile streama = gzopen(actual.toLatin1().data(),"rb");
            if (streama == NULL) QFAIL(QTest::toString("Could not open file '" + actual + "' for reading!"));
            gzFile streame = gzopen(expected.toLatin1().data(),"rb");
            if (streame == NULL) QFAIL(QTest::toString("Could not open file '" + expected + "' for reading!"));

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

                                    QVERIFY2(aline==eline, QTest::toString("Differing line '"  + QString::number(line_nr) + "' in file '" + QFileInfo(actual).fileName() + "'.\nA:" + aline + "\nE:" + eline));
                    ++line_nr;
            }

            //check if line counts differ
            if (!gzeof(streama)) QFAIL(QTest::toString("Actual file '" + actual + "' has more lines than expected file '" + expected + "'!"));
            if (!gzeof(streame)) QFAIL(QTest::toString("Actual file '" + actual + "' has less lines than expected file '" + expected + "'!"));

            //close streams
            gzclose(streama);
            gzclose(streame);
    }

    inline void removeLinesStartingWith(QString filename, QString prefix)
    {
            QFile file(filename);
            QStringList lines;

            //read input
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) QFAIL(QTest::toString("Could not open file '" + filename + " for reading!"));
            while(!file.atEnd())
            {
                    QString line = file.readLine();
                    if (line.startsWith(prefix)) continue;
                    lines.append(line);
            }
            file.close();

            //store output
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) QFAIL(QTest::toString("Could not open file '" + filename + " for writing!"));
            foreach(const QString& line, lines)
            {
                    file.write(line.toLatin1());
            }
            file.close();
    }

    inline void removeLinesContaining(QString filename, QString substr)
    {
            QFile file(filename);
            QStringList lines;

            //read input
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) QFAIL(QTest::toString("Could not open file '" + filename + " for reading!"));
            while(!file.atEnd())
            {
                    QString line = file.readLine();
                    if (line.contains(substr)) continue;
                    lines.append(line);
            }
            file.close();

            //store output
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) QFAIL(QTest::toString("Could not open file '" + filename + " for writing!"));
            foreach(const QString& line, lines)
            {
                    file.write(line.toLatin1());
            }
            file.close();
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

#define TFW_EXEC(toolname, arguments) \
	TFW::executeTool(toolname, arguments, false, __FILE__, __LINE__);

#define TFW_EXEC_FAIL(toolname, arguments) \
	TFW::executeTool(toolname, arguments, true, __FILE__, __LINE__);

#endif // TESTFRAMEWORK_H
