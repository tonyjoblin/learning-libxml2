// libxml2test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstring>

#include <string>
#include <iostream>
#include <memory>

#include <libxml/xmlreader.h>

using namespace std;

void DisplayLocalName(xmlTextReaderPtr reader) {
    auto name = xmlTextReaderLocalName(reader);
    cout << name << endl;
    xmlFree(name);
}

string GetAttribute(xmlTextReaderPtr reader, const string& name)
{
    xmlChar* value = xmlTextReaderGetAttribute(reader, (xmlChar*)name.c_str());
    string attributeValue((char*)value);
    xmlFree(value);
    return std::move(attributeValue);
}

shared_ptr<xmlChar> GetLocalName(xmlTextReaderPtr reader)
{
    shared_ptr<xmlChar> name(xmlTextReaderLocalName(reader), xmlFree);
    return std::move(name);
}

bool IsJourneyElement(xmlTextReaderPtr reader)
{
    auto name = GetLocalName(reader);
    return strncmp("Journey", (const char*)(name.get()), 7) == 0;
}

void processNode(xmlTextReaderPtr reader) {
    const int nodeType = xmlTextReaderNodeType(reader);
    switch (nodeType) {
    case XML_READER_TYPE_ELEMENT:
        if (IsJourneyElement(reader))
        {
            const string rid = GetAttribute(reader, "rid");
            const string uid = GetAttribute(reader, "uid");
            const string ssd = GetAttribute(reader, "ssd");
            cout << rid << "|" << ssd << "|" << uid << endl;
        }
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

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cerr << "Error, timetable file not specified." << endl;
        return 1;
    }
    
    string fileName = argv[1];
    
    streamFile(fileName);

    return 0;
}

