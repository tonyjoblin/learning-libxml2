// libxml2test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string>
#include <iostream>

#include <libxml/xmlreader.h>

using namespace std;

void DisplayLocalName(xmlTextReaderPtr reader) {
    auto name = xmlTextReaderLocalName(reader);
    cout << name << endl;
    xmlFree(name);
}

void processNode(xmlTextReaderPtr reader) {
    const int nodeType = xmlTextReaderNodeType(reader);
    switch (nodeType) {
    case XML_READER_TYPE_ELEMENT:
        DisplayLocalName(reader);
        break;
    };
}

void streamFile(const string& filename) {

    xmlTextReaderPtr reader;
    int ret;

    reader = xmlNewTextReaderFilename(filename.c_str());
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            processNode(reader);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) {
            cerr << "Failed to parse : " << filename << endl;
        }
    }
    else {
        cerr << "Unable to open " << filename << endl;
    }
}

int main()
{
    string fileName = R"(..\data\small_timetable.xml)";

    streamFile(fileName);

    return 0;
}

