// libxml2test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstring>

#include <string>
#include <iostream>
#include <memory>

#include <libxml/xmlreader.h>

using namespace std;

shared_ptr<xmlChar> GetLocalName(xmlTextReaderPtr reader)
{
    shared_ptr<xmlChar> name(xmlTextReaderLocalName(reader), xmlFree);
    return std::move(name);
}

shared_ptr<xmlChar> GetAttribute(xmlTextReaderPtr reader, const string& name)
{
    shared_ptr<xmlChar> value(xmlTextReaderGetAttribute(reader, (xmlChar*)name.c_str()), xmlFree);
    return std::move(value);
}

//string GetAttribute(xmlTextReaderPtr reader, const string& name)
//{
//    xmlChar* value = xmlTextReaderGetAttribute(reader, (xmlChar*)name.c_str());
//    string attributeValue((char*)value);
//    xmlFree(value);
//    return std::move(attributeValue);
//}

void DisplayLocalName(xmlTextReaderPtr reader) {
    auto name = GetLocalName(reader);
    cout << name << endl;
}

bool IsJourneyElement(xmlTextReaderPtr reader)
{
    auto name = GetLocalName(reader);
    return xmlStrncmp(BAD_CAST "Journey", name.get(), 7) == 0;
}

void processNode(xmlTextReaderPtr reader) {
    const int nodeType = xmlTextReaderNodeType(reader);
    switch (nodeType) {
    case XML_READER_TYPE_ELEMENT:
        if (IsJourneyElement(reader))
        {
            auto rid = GetAttribute(reader, "rid");
            auto uid = GetAttribute(reader, "uid");
            auto ssd = GetAttribute(reader, "ssd");
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

