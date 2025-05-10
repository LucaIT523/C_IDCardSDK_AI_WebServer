#include "MiParserJson.h"

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Exception.h>

using namespace Poco;
using namespace Poco::JSON;

typedef struct tagFieldName
{
	const char* m_szOrgName;
	const char* m_szName;
}ST_FIELD_NAME;


ST_FIELD_NAME lv_fieldFullProcess[] = {
//	Original Name					Show Name
	{"DocumentName",				"Document Name"},
	{"Surname And Given Names",		"Full Name"},
	{"Bank Card Valid Thru",		"Bank Card Validity"},
	{"MRZ Strings",					""},	//this field will be hide
	{NULL, NULL}
};

ST_FIELD_NAME lv_fieldMRZ[] = {
//	Original Name					Show Name
	{"Date of Birth",				"Date of Birth"},
	{"Date of Expiry",				"Date of Expiry"},
	{"Document Class Code",			"Document Class Code"},
	{"Document Number",				"Document Number"},
	{"Surname And Given Names",		"Full Name"},
	{"Issuing State Code",			"Issuing State Code"},
	{"Issuing State Name",			"Issuing State Name"},
	{"Personal Number",				"Personal Number"},
	{"Sex",							"Sex"},
	{"Surname",						"Surname"},
	{"MRZ Type",					"MRZ Type"},
	{"MRZ Strings",					"MRZ Strings"},
	{NULL, NULL}
};

ST_FIELD_NAME lv_fieldImage[] = {
//	Original Name					Show Name
	{"Barcode-VISUAL",				"Barcode"},
	{"Document front side-VISUAL",	"Document"},
	{"Portrait-BARCODE",			"Portrait-B"},
	{"Portrait-VISUAL",				"Portrait-V"},
	{"Signature-VISUAL",			"Signature"},
	{"Ghost portrait-VISUAL",		"Ghost portrait"},
	{"<unknown>-VISUAL",			"VISUAL"},
	{NULL, NULL}
};


const char* _getFieldName(const char* p_szName, ST_FIELD_NAME* p_pFieldList) {
	for (int i = 0; p_pFieldList[i].m_szOrgName != NULL; i++) {
		if (strcmp(p_szName, p_pFieldList[i].m_szOrgName) == 0) {
			return p_pFieldList[i].m_szName;
		}
	}
	return NULL;
}

const char* _getImageFieldName(const char* p_szName) {
	for (int i = 0; lv_fieldImage[i].m_szOrgName != NULL; i++) {
		if (strcmp(p_szName, lv_fieldImage[i].m_szOrgName) == 0) {
			return lv_fieldImage[i].m_szName;
		}
	}
	return p_szName;
}

std::string _convertFTtoCM(std::string p_strData) {
	int w_Pos = p_strData.find("cm");
	if (w_Pos <= 0) {
		int ft = 0, in = 0;
		sscanf_s(p_strData.c_str(), "%d ft %d in", &ft, &in);
		if (ft > 10) ft = 0;
		if (in > 10 || in < 0) in = 0;

		int cm = (((ft * 12) + in) * 2.54);
		char szRet[10]; memset(szRet, 0, sizeof(szRet));
		sprintf_s(szRet, 10, "%dcm", cm);
		return szRet;
	}
	else {
		return p_strData;
	}
}

Object::Ptr _getObject(Object::Ptr& p_pObj, const char* p_szKey) {
	Object::Ptr ret = NULL;
	try {
		ret = p_pObj->getObject(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return NULL;
	}
}

Array::Ptr _getArray(Object::Ptr& p_pObj, const char* p_szKey) {
	Array::Ptr ret = NULL;
	try {
		ret = p_pObj->getArray(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return NULL;
	}
}

std::string _getString(Object::Ptr& p_pObj, const char* p_szKey) {
	std::string ret = "";
	try {
		ret = p_pObj->getValue<std::string>(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return "";
	}
}

int _getNumber(Object::Ptr& p_pObj, const char* p_szKey) {
	int ret = 0;
	try {
		ret = p_pObj->getValue<int>(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return ret;
	}
}

void _addJsonTextItem(
	Object::Ptr& p_jsonRet,
	const char* p_szName, const char* p_szValue, const char* p_szLicName,
	ST_FIELD_NAME* p_pFieldNames, bool p_bOnlyInclude)
{
	const char* pszName = _getFieldName(p_szName, p_pFieldNames);
	if (pszName == NULL && p_bOnlyInclude) return;
	if (pszName != NULL && strcmp(pszName, "") == 0) return;
	std::string strNewName;
	if (pszName == NULL) {
		strNewName = p_szName;
	}
	else {
		strNewName = pszName;
	}

	if (p_szLicName[0] != 0) {
		strNewName = strNewName + "-" + p_szLicName;
	}
	std::string strValue = p_szValue;
	p_jsonRet->set(strNewName, strValue);
}

void extract_text_block(Object::Ptr& p_jsonMain, Object::Ptr& p_jsonRet, ST_FIELD_NAME* p_pFieldNames, bool p_bOnlyInclude, int p_nPageNo)
{
	Object::Ptr containerList = p_jsonMain->getObject("ContainerList");
	Array::Ptr listArray = containerList->getArray("List");
	int nTotalSize = listArray->size();

	for (int i = 0; i < nTotalSize; i++) {
		Object::Ptr obj = listArray->getObject(i);

		//.	for text result
		Object::Ptr objOneCandidate = _getObject(obj, "OneCandidate");
		Object::Ptr objText = _getObject(obj, "Text");

		if (objOneCandidate != nullptr) {
			std::string str_name;
			std::string str_value;

			std::string pageindex = _getString(obj, "page_idx");
			if (atoi(pageindex.c_str()) != p_nPageNo) continue;

			std::string value = _getString(objOneCandidate, "DocumentName");
			if (value != "") {
				_addJsonTextItem(p_jsonRet, "DocumentName", value.c_str(), "", p_pFieldNames, p_bOnlyInclude);
			}
		}
		else if (objText != nullptr) {
			Array::Ptr fieldList = _getArray(objText, "fieldList");
			if (fieldList == nullptr)
				continue;
			for (int j = 0; j < fieldList->size(); j++) {
				Object::Ptr field = fieldList->getObject(j);
				std::string fieldName = _getString(field, "fieldName");
				std::string lcidName = _getString(field, "lcidName");
				std::string value = _getString(field, "value");

				std::string value_org = "";
				bool bExist = false;

				Array::Ptr val_list = _getArray(field, "valueList");
				if (val_list != nullptr) {
					for (int t = 0; t < val_list->size(); t++) {
						Object::Ptr vals = val_list->getObject(t);
						std::string pageIndex = _getString(vals, "pageIndex");
						if (atoi(pageIndex.c_str()) != p_nPageNo) continue;
						bExist = true; break;
					}
					if (bExist == false) continue;
				}

				std::string str_name;
				if (lcidName == "")
					str_name = fieldName;
				else
					str_name = fieldName + "-" + lcidName;

				std::string str_value;
				if (fieldName == "") continue;
				if (value_org == "")
					str_value = value;
				else
					str_value = value_org;

				_addJsonTextItem(p_jsonRet, fieldName.c_str(), str_value.c_str(), lcidName.c_str(), p_pFieldNames, p_bOnlyInclude);
			}
		}
	}
}

void extract_image_block(Object::Ptr& p_jsonMain, Object::Ptr& p_jsonRet, int p_nPageNo)
{
	Object::Ptr containerList = p_jsonMain->getObject("ContainerList");
	Array::Ptr listArray = containerList->getArray("List");
	int nTotalSize = listArray->size();

	for (int i = 0; i < nTotalSize; i++) {
		Object::Ptr obj = listArray->getObject(i);

		//.	for text result
		Object::Ptr objImages = _getObject(obj, "Images");

		if (objImages != nullptr) {
			Array::Ptr fieldList = _getArray(objImages, "fieldList");
			if (fieldList == nullptr)
				continue;

			Object::Ptr imagesObject = new Object();

			for (int j = 0; j < fieldList->size(); j++) {

				Object::Ptr field_image = new Object();

				Object::Ptr field = fieldList->getObject(j);
				std::string fieldName = _getString(field, "fieldName");
				Array::Ptr valueList = _getArray(field, "valueList");

				for (int i = 0; i < valueList->size(); i++) {
					Object::Ptr valueListItem = valueList->getObject(i);

					std::string pageIndex = _getString(valueListItem, "pageIndex");
					if (atoi(pageIndex.c_str()) != p_nPageNo) continue;

					std::string sourcetype = _getString(valueListItem, "source");

					Object::Ptr fieldRect = valueListItem->getObject("fieldRect");
					std::string strImage;

					if (fieldRect != nullptr) {
						int Portrait_x1 = _getNumber(fieldRect, "left");
						int Portrait_x2 = _getNumber(fieldRect, "right");
						int Portrait_y1 = _getNumber(fieldRect, "top");
						int Portrait_y2 = _getNumber(fieldRect, "bottom");

						Object::Ptr portraitPositionObject = new Object();
						portraitPositionObject->set("x1", Portrait_x1);
						portraitPositionObject->set("x2", Portrait_x2);
						portraitPositionObject->set("y1", Portrait_y1);
						portraitPositionObject->set("y2", Portrait_y2);
						field_image->set("Position", portraitPositionObject);
					}
					strImage = _getString(valueListItem, "value");
					field_image->set("image", strImage);

					fieldName = _getImageFieldName((fieldName + "-" + sourcetype).c_str());
					p_jsonRet->set(fieldName, field_image);
				}
			}
		}
	}
}

std::string process_json_full_process(std::string p_strJson)
{
	Parser parser;
	auto result = parser.parse(p_strJson);
	Object::Ptr json_obj = result.extract<Object::Ptr>();

	Object::Ptr json_ret = new Object();
	Array::Ptr json_results = new Array();

	Object::Ptr json_ret_page1 = new Object();
	Object::Ptr json_ret_page2 = new Object();
	Object::Ptr json_mrz_page1 = new Object();
	Object::Ptr json_mrz_page2 = new Object();
	Object::Ptr json_img_page1 = new Object();
	Object::Ptr json_img_page2 = new Object();

	extract_text_block(json_obj, json_ret_page1, lv_fieldFullProcess, false, 0);
	extract_text_block(json_obj, json_mrz_page1, lv_fieldMRZ, true, 0);
	extract_image_block(json_obj, json_img_page1, 0);

	extract_text_block(json_obj, json_ret_page2, lv_fieldFullProcess, false, 1);
	extract_text_block(json_obj, json_mrz_page2, lv_fieldMRZ, true, 1);
	extract_image_block(json_obj, json_img_page2, 1);

	///////////////////////////////////////////////////////
	json_ret_page1->set("Images", json_img_page1);

	std::string strMrz = _getString(json_mrz_page1, "MRZ Strings");

	if (strMrz != "")
		json_ret_page1->set("MRZ", json_mrz_page1);
	///////////////////////////////////////////////////////

	if (json_ret_page2->size() || json_mrz_page2->size() || json_img_page2->size()) {
		json_ret_page2->set("Images", json_img_page2);

		std::string strMrz = _getString(json_mrz_page2, "MRZ Strings");

		if (strMrz != "")
			json_ret_page2->set("MRZ", json_mrz_page2);

		json_results->add(json_ret_page1);
		json_results->add(json_ret_page2);
		json_ret->set("result", json_results);
	}
	else {
		//delete json_ret;
		json_ret = json_ret_page1;
	}

	json_ret->set("Status", "Ok");
	//. 
	std::stringstream out_temp;
 	json_ret->stringify(out_temp, 2);
 	std::string ret = out_temp.str();
	return ret;
}

std::string process_json_credit_card(std::string p_strJson)
{
	Parser parser;
	auto result = parser.parse(p_strJson);
	Object::Ptr json_obj = result.extract<Object::Ptr>();

	Object::Ptr json_ret = new Object();
	Array::Ptr json_results = new Array();

	Object::Ptr json_ret_page1 = new Object();
	Object::Ptr json_ret_page2 = new Object();
	Object::Ptr json_img_page1 = new Object();
	Object::Ptr json_img_page2 = new Object();

	extract_text_block(json_obj, json_ret_page1, lv_fieldFullProcess, false, 0);
	extract_image_block(json_obj, json_img_page1, 0);

	extract_text_block(json_obj, json_ret_page2, lv_fieldFullProcess, false, 1);
	extract_image_block(json_obj, json_img_page2, 1);

	///////////////////////////////////////////////////////
	json_ret_page1->set("Images", json_img_page1);
	///////////////////////////////////////////////////////

	if (json_ret_page2->size() || json_img_page2->size()) {
		json_ret_page2->set("Images", json_img_page2);

		json_results->add(json_ret_page1);
		json_results->add(json_ret_page2);
		json_ret->set("result", json_results);
	}
	else {
		//delete json_ret;
		json_ret = json_ret_page1;
	}
	//. 
	json_ret->set("Status", "Ok");
	std::stringstream out_temp;
	json_ret->stringify(out_temp, 2);
	std::string ret = out_temp.str();
	return ret;
}

std::string process_json_mrz_barcode(std::string p_strJson)
{
	Parser parser;
	auto result = parser.parse(p_strJson);
	Object::Ptr json_obj = result.extract<Object::Ptr>();

	Object::Ptr json_ret = new Object();
	Array::Ptr json_results = new Array();

	Object::Ptr json_ret_page1 = new Object();
	Object::Ptr json_ret_page2 = new Object();
	Object::Ptr json_mrz_page1 = new Object();
	Object::Ptr json_mrz_page2 = new Object();
	Object::Ptr json_img_page1 = new Object();
	Object::Ptr json_img_page2 = new Object();

	extract_text_block(json_obj, json_mrz_page1, lv_fieldMRZ, true, 0);
	extract_image_block(json_obj, json_img_page1, 0);

	extract_text_block(json_obj, json_mrz_page2, lv_fieldMRZ, true, 1);
	extract_image_block(json_obj, json_img_page2, 1);

	///////////////////////////////////////////////////////
	json_ret_page1->set("Images", json_img_page1);
	std::string strMrz = _getString(json_mrz_page1, "MRZ Strings");
	if (strMrz != "")
		json_ret_page1->set("MRZ", json_mrz_page1);
	///////////////////////////////////////////////////////

	if (json_ret_page2->size() || json_mrz_page2->size() || json_img_page2->size()) {
		json_ret_page2->set("Images", json_img_page2);
		std::string strMrz = _getString(json_mrz_page2, "MRZ Strings");
		if (strMrz != "")
			json_ret_page2->set("MRZ", json_mrz_page2);

		json_results->add(json_ret_page1);
		json_results->add(json_ret_page2);
		json_ret->set("result", json_results);
	}
	else {
		//delete json_ret;
		json_ret = json_ret_page1;
	}

	json_ret->set("Status", "Ok");
	//. 
	std::stringstream out_temp;
	json_ret->stringify(out_temp, 2);
	std::string ret = out_temp.str();
	return ret;
}
