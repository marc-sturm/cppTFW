#ifndef TESTFRAMEWORKNGS_H
#define TESTFRAMEWORKNGS_H

#include "TestFramework.h"
#include "VcfFile.h"
#include "Settings.h"
#include "BamReader.h"

#define SKIP_IF_NO_TEST_NGSD()\
{\
	if (!NGSD::isAvailable(true))\
	{\
		TFW::skipped() = true;\
		TFW::message() = QByteArray("Test needs access to test instance of NGSD!\n")\
		+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}\
}

#define SKIP_IF_NO_PROD_NGSD()\
{\
	if (!NGSD::isAvailable(false))\
	{\
		TFW::skipped() = true;\
		TFW::message() = QByteArray("Test needs access to production instance of NGSD!\n")\
		+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}\
}

#define SKIP_IF_NO_PROD_GENLAB()\
{\
	if (!GenLabDB::isAvailable())\
	{\
		TFW::skipped() = true;\
		TFW::message() = QByteArray("Test needs access to production instance of GenLab!\n")\
		+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}\
}

#define SKIP_IF_NO_HG38_GENOME()\
{\
	if (Settings::string("reference_genome", true)=="")\
	{\
		TFW::skipped() = true;\
		TFW::message() = QByteArray("Test needs the HG38 reference genome!\n")\
		+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}\
}


#define SKIP_IF_NO_HG19_GENOME()\
{\
	if (Settings::string("reference_genome_hg19", true)=="")\
	{\
		TFW::skipped() = true;\
		TFW::message() = QByteArray("Test needs the HG19 reference genome!\n")\
		+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}\
}

#define VCF_IS_VALID(vcf_file)\
{\
    QString ref_file = Settings::string("reference_genome", true);\
    if (ref_file!="")\
    {\
        QString output;\
        QTextStream out_stream(&output);\
        if (!VcfFile::isValid(vcf_file, ref_file, out_stream))\
        {\
            TFW::failed() = true;\
            TFW::message() = "CHECK_VCF_VALID(" + QByteArray(#vcf_file) + ") failed\n"\
            + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
            + "message  : " + output.toUtf8();\
            return;\
        }\
    }\
}

#define VCF_IS_VALID_HG19(vcf_file)\
{\
	QString ref_file = Settings::string("reference_genome_hg19", true);\
	if (ref_file!="")\
	{\
		QString output;\
		QTextStream out_stream(&output);\
		if (!VcfFile::isValid(vcf_file, ref_file, out_stream))\
		{\
			TFW::failed() = true;\
			TFW::message() = "CHECK_VCF_VALID_HG19(" + QByteArray(#vcf_file) + ") failed\n"\
			+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
			+ "message  : " + output.toUtf8();\
			return;\
		}\
	}\
}


#define BAM_TO_TEXT(bam_file, txt_file)\
{\
	QFile out(txt_file);\
	out.open(QIODevice::WriteOnly);\
	BamReader reader(bam_file);\
	BamAlignment al;\
	while(reader.getNextAlignment(al))\
	{\
		QByteArrayList line;\
		line << reader.chromosome(al.chromosomeID()).str() << QByteArray::number(al.start()) << QByteArray::number(al.end());\
		line << QByteArray::number(al.mappingQuality()) << QByteArray::number(al.insertSize()) << al.cigarDataAsString();\
		line << al.bases();\
		line << al.qualities();\
		out.write(line.join("\t") + "\n");\
	}\
	out.close();\
}

#endif // TESTFRAMEWORK_H
