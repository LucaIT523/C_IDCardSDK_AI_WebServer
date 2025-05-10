#include "MIServer.h"
#include "MiParserJson.h"
//#include <atltime.h>


ST_RESPONSE* lv_pstRes = NULL;
#define LD_MAX_TRIAL_COUNT 100
int lv_nTrialCount = 50 * 2;

std::string replaceAll(std::string original, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = original.find(search, pos)) != std::string::npos) {
		original.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return original;
}

HTTPResponse::HTTPStatus sendPostRequest(const std::string& url, const std::string& payload, std::string& p_strRsp)
{
	URI uri(url);
	HTTPClientSession session(uri.getHost(), uri.getPort());

	HTTPRequest request(HTTPRequest::HTTP_POST, uri.getPathAndQuery(), HTTPMessage::HTTP_1_1);
	request.setContentType("application/json");
	request.setContentLength(payload.length());

	std::ostream& os = session.sendRequest(request);
	os << payload;

	HTTPResponse response;
	std::istream& rs = session.receiveResponse(response);

	std::string responseData;
	StreamCopier::copyToString(rs, responseData);
	p_strRsp = responseData;

	return response.getStatus();

}

unsigned int TF_READ_LIC(void*) {
	INT64 nSts = 0;
	ST_RESPONSE* p = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));

	while (TRUE)
	{
		Sleep(10 * 1000);

		memset(p, 0, sizeof(ST_RESPONSE));
		if ((nSts = mil_read_license(p)) <= 0) {
			if (lv_pstRes != NULL) free(lv_pstRes);
			lv_pstRes = NULL;
		}
		else {
			if (lv_pstRes == NULL) {
				lv_pstRes = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));
			}
			memcpy(lv_pstRes, p, sizeof(ST_RESPONSE));
		}
	}
	free(p); // Clean up allocated memory
}

void ClaHTTPServerWrapper::launch() {

	if (lv_pstRes == NULL) {
		lv_pstRes = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));
		if (mil_read_license(lv_pstRes) <= 0) {
			free(lv_pstRes);
			lv_pstRes = NULL;
		}
	}

	DWORD dwTID = 0;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TF_READ_LIC, NULL, 0, &dwTID);

	run();
}

void MyRequestHandler::OnVersion(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	char szOut[MAX_PATH]; memset(szOut, 0, sizeof(szOut));
	sprintf_s(szOut, "Version : %s\nUpdate : %s", GD_ID_VERSION, GD_ID_UPDATE);
	ostr << szOut;
}
void MyRequestHandler::OnProcessProc(HTTPServerRequest& request, HTTPServerResponse& response, Poco::Dynamic::Var procName, int base64, int nMutiPage/* = 0*/)
{
	// Get the current time point
	auto now = std::chrono::system_clock::now();

	// Convert the time point to a time_t object
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
	if (lv_pstRes == NULL || lv_pstRes->m_lExpire < now_c) {
		OnNoLicense(request, response); 
		return;
	}

	int		InputImageCnt = 1;
	if (nMutiPage == 1) {
		InputImageCnt = 2;
	}
	else {
		InputImageCnt = 1;
	}

	Object::Ptr imageData;// = new Object;
	Object::Ptr listItem;// = new Object;
	Array::Ptr list = new Array;


	if (base64 == 0) {
		MyPartHandler hPart;
		Poco::Net::HTMLForm form(request, request.stream(), hPart);

		Object::Ptr jsonResponse = new Poco::JSON::Object();

		// Alternatively, you can use a traditional for loop
		for (size_t i = 0; i < InputImageCnt; i ++) {
			std::string image64 = hPart.uploadedFiles().at(i).base64Data;
			image64 = replaceAll(image64, "\r\n", "");
			imageData = new Object;
			imageData->set("image", image64);
			listItem = new Object;
			listItem->set("ImageData", imageData);
			listItem->set("page_idx", i);
			list->add(listItem);
		}
	}
	else {
		std::istream& input = request.stream();
		std::ostringstream ss;
		StreamCopier::copyStream(input, ss);
		std::string data = ss.str();

		Parser parser;
		auto result = parser.parse(data);

		if (InputImageCnt > 1) {
			// Extract JSON object
			Poco::JSON::Object::Ptr jsonObject = result.extract<Poco::JSON::Object::Ptr>();
			// Get the values
			std::string image1 = jsonObject->getValue<std::string>("image1");
			std::string image2 = jsonObject->getValue<std::string>("image2");

			imageData = new Object;
			imageData->set("image", image1);
			listItem = new Object;
			listItem->set("ImageData", imageData);
			listItem->set("page_idx", 0);
			list->add(listItem);

			imageData = new Object;
			imageData->set("image", image2);
			listItem = new Object;
			listItem->set("ImageData", imageData);
			listItem->set("page_idx", 1);
			list->add(listItem);
		}
		else {
			// Extract JSON object
			Poco::JSON::Object::Ptr jsonObject = result.extract<Poco::JSON::Object::Ptr>();
			// Get the values
			std::string image = jsonObject->getValue<std::string>("image");
			imageData = new Object;
			imageData->set("image", image);
			listItem = new Object;
			listItem->set("ImageData", imageData);
			listItem->set("page_idx", 0);
			list->add(listItem);
		}
	}

	// JSON payload to send in the POST request
	Object::Ptr processParam = new Object;
	processParam->set("scenario", procName);
	processParam->set("dateFormat", "yyyy-mm-dd");
	//processParam->set("alreadyCropped", true);

	Object::Ptr root = new Object;
	root->set("processParam", processParam);
	root->set("List", list);

	std::ostringstream oss;
	Stringifier::stringify(root, oss);
	std::string payload = oss.str();

	try
	{
		// Send POST request
		std::string strRsp;
		HTTPServerResponse::HTTPStatus status = sendPostRequest(GD_URL_REG, payload, strRsp);
		if (status == HTTPResponse::HTTP_OK) {

			std::string out;
			if (procName == "FullProcess") {
				out = process_json_full_process(strRsp);
			}
			else if (procName == "CreditCard") {
				out = process_json_credit_card(strRsp);
			}
			else if (procName == "MrzOrBarcodeOrOcr") {
				out = process_json_mrz_barcode(strRsp);
			}
			out = replaceAll(out, "\r\n", "");
			out = replaceAll(out, "\n", "");

			response.setStatus(status);
			response.setContentType("application/json");
			response.setContentLength(out.length());

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.send() << out.c_str();
		}
		else {
			response.setStatus(status);
			response.setContentType("application/json");

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.setContentLength(strRsp.length());
			response.send() << strRsp.c_str();
		}
	}
	catch (const Exception& ex)
	{
		response.setStatus(HTTPResponse::HTTP_CONFLICT);
		response.setContentType("application/json");

		response.set("Access-Control-Allow-Origin", "*");
		response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
		response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

		response.setContentLength(ex.displayText().length());
		response.send() << ex.displayText();
	}
}

void MyRequestHandler::OnUnknown(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	ostr << "Not found";
}

void MyRequestHandler::OnNoLicense(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	ostr << "Please input license.";
}

void MyRequestHandler::OnStatus(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	

	if (lv_pstRes == NULL) {
		//. no license
		ostr << "License not found";
	}
	else {
		time_t t = lv_pstRes->m_lExpire;
		struct tm timeinfo;
		localtime_s(&timeinfo, &t);
		char szTime[260]; memset(szTime, 0, sizeof(szTime));
		if (lv_pstRes->m_lExpire < 32503622400) {
			strftime(szTime, 260, "License valid : %Y-%m-%d", &timeinfo);
		}
		else {
			sprintf_s(szTime, 260, "License valid : NO LIMIT");
		}
		ostr << szTime;
	}
}