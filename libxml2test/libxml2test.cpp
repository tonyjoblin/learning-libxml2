// libxml2test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstring>

#include <string>
#include <iostream>
#include <memory>
#include <iomanip>
#include <sstream>

#include <libxml/xmlreader.h>

#include <boost/optional.hpp>

using namespace std;

typedef shared_ptr<xmlChar> pstring;

pstring GetLocalName(xmlTextReaderPtr& reader)
{
    shared_ptr<xmlChar> name(xmlTextReaderLocalName(reader), xmlFree);
    return std::move(name);
}

// May return null
pstring GetAttribute(xmlTextReaderPtr& reader, const string& name)
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

void DisplayLocalName(xmlTextReaderPtr& reader) {
    auto name = GetLocalName(reader);
    cout << name << endl;
}

bool IsJourneyElement(xmlTextReaderPtr& reader)
{
    auto name = GetLocalName(reader);
    return xmlStrncmp(BAD_CAST "Journey", name.get(), 7) == 0;
}

enum StatesType {
    LookingForJourney,
    LookingForDeparture,
    LookingForArrival
};

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

struct StateDataType
{
public:
    StatesType  state;
    pstring     tripRid;
    pstring     tripSsd;
    pstring     tripUid;
    pstring     lastStationTiploc;
    time_t      lastBoardingTime;
    time_t      lastPassingTime;
};

StateDataType ProcessJourneyStartElement(xmlTextReaderPtr& reader, const StateDataType currentState) {
    auto rid = GetAttribute(reader, "rid");
    auto uid = GetAttribute(reader, "uid");
    auto ssd = GetAttribute(reader, "ssd");

    StateDataType nextState(currentState);
    nextState.tripRid = rid;
    nextState.tripSsd = ssd;
    nextState.tripUid = uid;
    nextState.state = LookingForDeparture;
    return nextState;
}

time_t AdjustStopsPastMidnight(time_t stopTime, time_t lastPassingTime) {
    const int tenHours = 10 * 60 * 60;
    const int twentyFourHours = 24 * 60 * 60;
    if (stopTime + tenHours < lastPassingTime) {
        return stopTime + twentyFourHours;
    }
    return stopTime;
}

time_t GetTime(const pstring& tripDate, const pstring& stopTime) {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    istringstream dateStream((char*)tripDate.get());
    dateStream >> get_time(&tm, "%Y-%m-%d");
    istringstream timeStream((char*)stopTime.get());
    timeStream >> get_time(&tm, "%H:%M");
    return mktime(&tm);
}

boost::optional<time_t> GetStopTime(xmlTextReaderPtr& reader, const char* timeType, const pstring& tripDate, const time_t lastPassingTime)
{
    pstring passingTime = GetAttribute(reader, timeType);
    if (passingTime == nullptr) {
        return boost::none;
    }
    time_t t = GetTime(tripDate, passingTime);
    return AdjustStopsPastMidnight(t, lastPassingTime);
}

boost::optional<time_t> GetDepartureTime(xmlTextReaderPtr& reader, const pstring& tripDate, const time_t lastPassingTime)
{
    return std::move(GetStopTime(reader, "ptd", tripDate, lastPassingTime));
}

boost::optional<time_t> GetPassingTime(xmlTextReaderPtr& reader, const pstring& tripDate, const time_t lastPassingTime)
{
    return std::move(GetStopTime(reader, "wtp", tripDate, lastPassingTime));
}

boost::optional<time_t> GetArrivalTime(xmlTextReaderPtr& reader, const pstring& tripDate, const time_t lastPassingTime)
{
    return std::move(GetStopTime(reader, "pta", tripDate, lastPassingTime));
}

pstring GetStopTiploc(xmlTextReaderPtr& reader) {
    return std::move(GetAttribute(reader, "ftl"));
}

StateDataType ProcessNextEvent(xmlTextReaderPtr& reader, const StateDataType currentState);

StateDataType LookingForJourneyStateHandler(xmlTextReaderPtr& reader, const StateDataType currentState) {
    const int nodeType = xmlTextReaderNodeType(reader);
    switch (nodeType) {
    case StartElement:
        if (IsJourneyElement(reader))
        {
            return ProcessJourneyStartElement(reader, currentState);
        }
        return currentState;
        break;
    default:
        break;
    };
    StateDataType nextState(currentState);
    return nextState;
}

enum TripStopType {
    Unknown = 0,
    OR,
    IP,
    PP,
    DT
};

TripStopType GetTripStopType(xmlTextReaderPtr& reader) {
    auto name = GetLocalName(reader);
    if (xmlStrncmp(BAD_CAST "OR", name.get(), 2) == 0) {
        return OR;
    }
    else if (xmlStrncmp(BAD_CAST "IP", name.get(), 2) == 0) {
        return IP;
    }
    else if (xmlStrncmp(BAD_CAST "PP", name.get(), 2) == 0) {
        return PP;
    }
    else if (xmlStrncmp(BAD_CAST "DT", name.get(), 2) == 0) {
        return DT;
    }
    return Unknown;
}

StateDataType LookingForDepatureProcessStop(xmlTextReaderPtr& reader, const StateDataType currentState) {
    boost::optional<time_t> departureTime = GetDepartureTime(reader, currentState.tripSsd, currentState.lastPassingTime);
    if (departureTime == boost::none) {
        return currentState;
    }
    StateDataType nextState(currentState);
    nextState.lastBoardingTime = *departureTime;
    nextState.lastPassingTime = *departureTime;
    nextState.lastStationTiploc = GetStopTiploc(reader);
    nextState.state = LookingForArrival;
    return std::move(nextState);
}

string FormatTimet(time_t t) {
    char buffer[20];
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    localtime_s(&tm, &t);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", &tm);
    string time = buffer;
    return std::move(time);
}

void PrintConnectionInformation(
    const time_t   departureTime,
    const pstring& origin,
    const time_t   arrivalTime,
    const pstring& destination,
    const pstring& tripRid,
    const pstring& tripUid
) {
    const char s = '|';
    cout
        << departureTime << s
        << origin << s
        << arrivalTime << s
        << destination << s
        << tripRid << s
        << tripUid << endl;
}

StateDataType LookingForArrivalProcessStop(xmlTextReaderPtr& reader, const StateDataType currentState) {
    boost::optional<time_t> arrivalTime = GetArrivalTime(reader, currentState.tripSsd, currentState.lastPassingTime);
    if (arrivalTime == boost::none) {
        return currentState;
    }
    pstring stopTiploc = GetStopTiploc(reader);
    PrintConnectionInformation(
        currentState.lastBoardingTime,
        currentState.lastStationTiploc,
        *arrivalTime,
        stopTiploc,
        currentState.tripRid,
        currentState.tripUid
    );
    StateDataType nextState(currentState);
    nextState.lastPassingTime = *arrivalTime;
    nextState.state = LookingForDeparture;
    return std::move(ProcessNextEvent(reader, nextState));
}

StateDataType UpdatePassingTime(xmlTextReaderPtr& reader, const StateDataType currentState) {
    boost::optional<time_t> passingTime = GetPassingTime(reader, currentState.tripSsd, currentState.lastPassingTime);
    if (passingTime == boost::none) {
        return currentState;
    }
    StateDataType nextState(currentState);
    nextState.lastPassingTime = *passingTime;
    return std::move(nextState);
}

StateDataType LookingForDepartureHandler(xmlTextReaderPtr& reader, const StateDataType currentState) {
    const int nodeType = xmlTextReaderNodeType(reader);
    switch (nodeType) {
    case EndElement:
        if (IsJourneyElement(reader))
        {
            StateDataType nextState(currentState);
            nextState.state = LookingForJourney;
            return nextState;
        }
        break;
    case StartElement: {
            auto stopType = GetTripStopType(reader);
            switch (stopType) {
            case OR:
            case IP:
            case DT:
                return LookingForDepatureProcessStop(reader, currentState);
                break;
            case PP:
                return UpdatePassingTime(reader, currentState);
                break;
            default:
                break;
            };
        }
        break;
    default:
        break;
    };
    return currentState;
}

StateDataType LookingForArrivalHandler(xmlTextReaderPtr& reader, const StateDataType currentState) {
    const int nodeType = xmlTextReaderNodeType(reader);
    switch (nodeType) {
    case EndElement:
        if (IsJourneyElement(reader))
        {
            StateDataType nextState(currentState);
            nextState.state = LookingForJourney;
            return nextState;
        }
        break;
    case StartElement: {
        auto stopType = GetTripStopType(reader);
        switch (stopType) {
        case OR:
        case IP:
        case DT:
            return LookingForArrivalProcessStop(reader, currentState);
            break;
        case PP:
            return UpdatePassingTime(reader, currentState);
            break;
        default:
            break;
        };
    }
                       break;
    default:
        break;
    };
    return currentState;
}

StateDataType ProcessNextEvent(xmlTextReaderPtr& reader, const StateDataType currentState) {
    switch (currentState.state) {
    case LookingForJourney:
        return LookingForJourneyStateHandler(reader, currentState);
        break;
    case LookingForDeparture:
        return LookingForDepartureHandler(reader, currentState);
        break;
    case LookingForArrival:
        return LookingForArrivalHandler(reader, currentState);
        break;
    default:
        cerr << "Error: Unhandled state" << currentState.state << endl;
        exit(1);
    }
}

StateDataType DebugProcessNextEvent(xmlTextReaderPtr& reader, const StateDataType currentState) {
    const int nodeType = xmlTextReaderNodeType(reader);
    auto name = GetLocalName(reader);
    if (name != nullptr) {
        cout << name << " " << nodeType << endl;
    }
    else {
        cout << nodeType << endl;
    }
    return currentState;
}

void streamFile(const string& filename) {

    xmlTextReaderPtr reader;
    int ret;

    StateDataType state;
    state.state = LookingForJourney;
    state.lastPassingTime = 0;

    reader = xmlNewTextReaderFilename(filename.c_str());
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            state = ProcessNextEvent(reader, state);
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

