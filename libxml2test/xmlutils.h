#pragma once

#ifndef __XMLUTILS_H__
#define __XMLUTILS_H__

#include <libxml/xmlreader.h>
#include <memory>

enum NodeTypes {
    StartElement = 1,
    Attribute = 2,
    Text = 3,
    CDate = 4,
    EntityRef = 5,
    EntityDecl = 6,
    ProcessingInstruction = 7,
    Comment = 8,
    Document = 9,
    Dtd = 10,
    DocumentFragment = 11,
    Notation = 12,
    EndElement = 15
};

typedef std::shared_ptr<xmlChar> pstring;

pstring GetLocalName(xmlTextReaderPtr& reader)
{
    pstring name(xmlTextReaderLocalName(reader), xmlFree);
    return std::move(name);
}

// May return null
pstring GetAttribute(xmlTextReaderPtr& reader, const std::string& name)
{
    pstring value(xmlTextReaderGetAttribute(reader, (xmlChar*)name.c_str()), xmlFree);
    return std::move(value);
}

// May return null
pstring GetAttribute(xmlTextReaderPtr& reader, const char* name)
{
    pstring value(xmlTextReaderGetAttribute(reader, BAD_CAST name), xmlFree);
    return std::move(value);
}

#endif 
