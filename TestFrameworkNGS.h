#ifndef TESTFRAMEWORKNGS_H
#define TESTFRAMEWORKNGS_H

#include "TestFramework.h"
#include "VcfFile.h"
#include "Settings.h"

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

#endif // TESTFRAMEWORK_H
