#include "wirbelscan.h"

#include <sstream>

using namespace std;
using namespace WIRBELSCAN_SERVICE;

void WirbelscanResponder::reply(ostream& out, cxxtools::http::Request& request,
		cxxtools::http::Reply& reply) {

	QueryHandler::addHeader(reply);

	if ((wirbelscan = cPluginManager::GetPlugin("wirbelscan")) == NULL) {
		reply.httpReturn(403U, "wirbelscan was not found - pls install.");
		return;
	}

	if (request.method() != "GET") {
		reply.httpReturn(403U, "To retrieve information use the GET method!");
		return;
	}
	static cxxtools::Regex countriesRegex("/wirbelscan/countries.json");
	static cxxtools::Regex satellitesRegex("/wirbelscan/satellites.json");
	static cxxtools::Regex getStatusRegex("/wirbelscan/getStatus.json");
	static cxxtools::Regex doCmdRegex("/wirbelscan/doCommand.json/*");
	static cxxtools::Regex getSetupRegex("/wirbelscan/getSetup.json");
	static cxxtools::Regex setSetupRegex("/wirbelscan/setSetup.json");

	if (countriesRegex.match(request.url())) {
		replyCountries(out, request, reply);
	} else if (satellitesRegex.match(request.url())) {
		replySatellites(out, request, reply);
	} else if (getStatusRegex.match(request.url())) {
		replyGetStatus(out, request, reply);
	} else if (doCmdRegex.match(request.url())) {
		replyDoCmd(out, request, reply);
	} else if (getSetupRegex.match(request.url())) {
		replyGetSetup(out, request, reply);
	} else if (setSetupRegex.match(request.url())) {
		replySetSetup(out, request, reply);
	} else {
		replyGetStatus(out, request, reply);
	}
}

void WirbelscanResponder::replyCountries(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{

	QueryHandler q("/countries", request);

	CountryList* countryList;

	reply.addHeader("Content-Type", "application/json; charset=utf-8");
	countryList = (CountryList*) new JsonCountryList(&out);

	countryList->init();

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SCountry;

	cPreAllocBuffer countryBuffer;
	countryBuffer.size = 0;
	countryBuffer.count = 0;
	countryBuffer.buffer = NULL;

	wirbelscan->Service(cmd.str().c_str(), &countryBuffer); // query buffer size.

	SListItem *cbuf = (SListItem *) malloc(
			countryBuffer.size * sizeof(SListItem)); // now, allocate memory.
	countryBuffer.buffer = cbuf; // assign buffer
	wirbelscan->Service(cmd.str().c_str(), &countryBuffer); // fill buffer with values.

	for (unsigned int i = 0; i < countryBuffer.count; ++i)
	{
		countryList->addCountry(cbuf++);
	}

	countryList->setTotal(countryBuffer.count);
	countryList->finish();
	delete countryList;
}

void WirbelscanResponder::replyGetStatus(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
    QueryHandler::addHeader(reply);
	QueryHandler q("/getStatus", request);

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SStatus;

	cWirbelscanStatus statusBuffer;

	wirbelscan->Service(cmd.str().c_str(), &statusBuffer); // query buffer size.

	StreamExtension se(&out);

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");

		cxxtools::JsonSerializer serializer(*se.getBasicStream());

		serializer.serialize((int)statusBuffer.status, "status");
		if (statusBuffer.status == StatusScanning)
		{
			serializer.serialize(statusBuffer.curr_device, "currentDevice");
			serializer.serialize((int) statusBuffer.progress, "progress");
			serializer.serialize((int) statusBuffer.strength, "strength");
			serializer.serialize(statusBuffer.transponder, "transponder");
			serializer.serialize((int) statusBuffer.numChannels, "numChannels");
			serializer.serialize((int) statusBuffer.newChannels, "newChannels");
			serializer.serialize((int) statusBuffer.nextTransponders,
					"nextTransponder");
		}
		serializer.finish();
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void WirbelscanResponder::replyGetSetup(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
    QueryHandler::addHeader(reply);
	QueryHandler q("/getSetup", request);

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SSetup;

	cWirbelscanScanSetup setupBuffer;

	wirbelscan->Service(cmd.str().c_str(), &setupBuffer); // query buffer size.

	StreamExtension se(&out);

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");

		cxxtools::JsonSerializer serializer(*se.getBasicStream());

		serializer.serialize((int) setupBuffer.verbosity, "verbosity");
		serializer.serialize((int) setupBuffer.logFile, "logFile");
		serializer.serialize((int) setupBuffer.DVB_Type, "DVB_Type");
		serializer.serialize((int) setupBuffer.DVBT_Inversion,
				"DVBT_Inversion");
		serializer.serialize((int) setupBuffer.DVBC_Inversion,
				"DVBC_Inversion");
		serializer.serialize((int) setupBuffer.DVBC_Symbolrate,
				"DVBC_Symbolrate");
		serializer.serialize((int) setupBuffer.DVBC_QAM, "DVBC_QAM");
		serializer.serialize((int) setupBuffer.CountryId, "CountryId");
		serializer.serialize((int) setupBuffer.SatId, "SatId");
		serializer.serialize((int) setupBuffer.scanflags, "scanflags");
		serializer.serialize((int) setupBuffer.ATSC_type, "ATSC_type");

		serializer.finish();
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void WirbelscanResponder::replySetSetup(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
    QueryHandler::addHeader(reply);
	QueryHandler q("/setSetup", request);
	cxxtools::QueryParams options;

	options.parse_url(request.qparams());

	std::stringstream getcmd;
	getcmd << SPlugin << "Get" << SSetup;

	cWirbelscanScanSetup setupBuffer;

	wirbelscan->Service(getcmd.str().c_str(), &setupBuffer); // query buffer size.

	if (options.has("verbosity"))
	{
		setupBuffer.verbosity = q.getOptionAsInt("verbosity");
	}

	if (options.has("logFile"))
	{
		setupBuffer.logFile = q.getOptionAsInt("logFile");
	}

	if (options.has("DVB_Type"))
	{
		setupBuffer.DVB_Type = q.getOptionAsInt("DVB_Type");
	}

	if (options.has("DVBT_Inversion"))
	{
		setupBuffer.DVBT_Inversion = q.getOptionAsInt("DVBT_Inversion");
	}

	if (options.has("DVBC_Inversion"))
	{
		setupBuffer.DVBC_Inversion = q.getOptionAsInt("DVBC_Inversion");
	}

	if (options.has("DVBC_Symbolrate"))
	{
		setupBuffer.DVBC_Symbolrate = q.getOptionAsInt("DVBC_Symbolrate");
	}

	if (options.has("DVBC_QAM"))
	{
		setupBuffer.DVBC_QAM = q.getOptionAsInt("DVBC_QAM");
	}

	if (options.has("CountryId"))
	{
		setupBuffer.CountryId = q.getOptionAsInt("CountryId");
	}

	if (options.has("SatId"))
	{
		setupBuffer.SatId = q.getOptionAsInt("SatId");
	}

	if (options.has("scanflags"))
	{
		setupBuffer.scanflags = q.getOptionAsInt("scanflags");
	}

	if (options.has("ATSC_type"))
	{
		setupBuffer.ATSC_type = q.getOptionAsInt("ATSC_type");
	}

	std::stringstream setcmd;
	setcmd << SPlugin << "Set" << SSetup;
	wirbelscan->Service(setcmd.str().c_str(), &setupBuffer); // query buffer size.

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void WirbelscanResponder::replyDoCmd(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
    QueryHandler::addHeader(reply);
	QueryHandler q("/doCommand", request);

	cWirbelscanCmd commandBuffer;

	commandBuffer.cmd = (s_cmd)q.getOptionAsInt("command");

	std::stringstream cmd;
	cmd << SPlugin << SCommand;

	wirbelscan->Service(cmd.str().c_str(), &commandBuffer); // query buffer size.

	StreamExtension se(&out);

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");

		cxxtools::JsonSerializer serializer(*se.getBasicStream());

		serializer.serialize(commandBuffer.replycode, "replycode");
		serializer.finish();
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void operator<<= (cxxtools::SerializationInfo& si, const SerListItem& l)
{
  si.addMember("id") <<= l.id;
  si.addMember("shortName") <<= l.shortName;
  si.addMember("fullName") <<= l.fullName;
}

CountryList::CountryList(ostream* _out)
{
  s = new StreamExtension(_out);
}

CountryList::~CountryList()
{
  delete s;
}

void JsonCountryList::addCountry(SListItem* country)
{
  SerListItem serCountry;

  serCountry.id = country->id;
  serCountry.shortName = StringExtension::UTF8Decode(country->short_name);
  serCountry.fullName = StringExtension::UTF8Decode(country->full_name);

  SerCountries.push_back(serCountry);
}

void JsonCountryList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(SerCountries, "countries");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void WirbelscanResponder::replySatellites(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{

	QueryHandler q("/satellites", request);

	SatelliteList* satelliteList;

	reply.addHeader("Content-Type", "application/json; charset=utf-8");
	satelliteList = (SatelliteList*) new JsonSatelliteList(&out);

	satelliteList->init();

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SSat;

	cPreAllocBuffer satelliteBuffer;
	satelliteBuffer.size = 0;
	satelliteBuffer.count = 0;
	satelliteBuffer.buffer = NULL;

	wirbelscan->Service(cmd.str().c_str(), &satelliteBuffer); // query buffer size.

	SListItem *cbuf = (SListItem *) malloc(
			satelliteBuffer.size * sizeof(SListItem)); // now, allocate memory.
	satelliteBuffer.buffer = cbuf; // assign buffer
	wirbelscan->Service(cmd.str().c_str(), &satelliteBuffer); // fill buffer with values.

	for (unsigned int i = 0; i < satelliteBuffer.count; ++i)
	{
		satelliteList->addSatellite(cbuf++);
	}

	satelliteList->setTotal(satelliteBuffer.count);
	satelliteList->finish();
	delete satelliteList;
}

SatelliteList::SatelliteList(ostream* _out)
{
  s = new StreamExtension(_out);
}

SatelliteList::~SatelliteList()
{
  delete s;
}

void JsonSatelliteList::addSatellite(SListItem* satellite)
{
  SerListItem serSatellite;

  serSatellite.id = satellite->id;
  serSatellite.shortName = StringExtension::UTF8Decode(satellite->short_name);
  serSatellite.fullName = StringExtension::UTF8Decode(satellite->full_name);

  SerSatellites.push_back(serSatellite);
}

void JsonSatelliteList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(SerSatellites, "satellites");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}