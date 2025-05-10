# 

<div align="center">
   <h1>C++_IDCardSDK_AI_WebServer (Windows, Linux, Docker)</h1>
</div>



### **1. Core Architecture**

This code implements a **ID Card Processing Server** handling three primary scenarios:

1. **Full Document Processing** (`FullProcess`)
2. **Credit Card Recognition** (`CreditCard`)
3. **MRZ/Barcode/OCR Processing** (`MrzOrBarcodeOrOcr`)

------

### **2. Key Components**

#### **2.1 License Management System**

```
// Persistent license check (10s interval)
unsigned int TF_READ_LIC(void*) {
    while(TRUE) {
        Sleep(10*1000);
        mil_read_license(p); // License validation
        memcpy(lv_pstRes, p, ...); // Update global status
    }
}

// Runtime validation
if (lv_pstRes->m_lExpire < now_c) {
    OnNoLicense(...); // Block unauthorized access
}
```

#### **2.2 Multi-Format Input Handling**

| Input Type      | Processing Method               |
| --------------- | ------------------------------- |
| Multipart Form  | `MyPartHandler` file upload     |
| JSON Base64     | Direct payload parsing          |
| Multiple Images | `nMutiPage=1` with page indexes |

------

### **3. Service Workflow**

```
1. License Validity Check → 
2. Input Parsing (Form/JSON) → 
3. Payload Construction → 
4. Backend Service Proxy (GD_URL_REG) → 
5. Response Transformation → 
6. CORS-enabled Output
```

------

### **4. Core Processing Logic**

#### **4.1 Image Handling**

- **Base64 Sanitization**:

  ```
  replaceAll(image64, "\r\n", ""); // Remove line breaks
  ```

- **Multi-page Support**:

  ```
  listItem->set("page_idx", i); // 0/1 indexing
  ```

#### **4.2 Scenario Configuration**

```
cppCopyprocessParam->set("scenario", procName); // FullProcess/CreditCard/etc
processParam->set("dateFormat", "yyyy-mm-dd"); // Standardized output
```

#### **4.3 Response Processing**

```
// Scenario-specific transformers:
process_json_full_process()
process_json_credit_card() 
process_json_mrz_barcode()
```

------

### **5. Security Implementation**

#### **5.1 Access Control**

- 10-second license check interval
- Expiration timestamp validation (UNIX time)
- Graceful license expiration handling

#### **5.2 CORS Policy**

```
cppCopyresponse.set("Access-Control-Allow-Origin", "*");
response.set("Access-Control-Allow-Methods", "GET, POST...");
```

------

### **6. Technical Parameters**

| Parameter        | Value        | Description            |
| ---------------- | ------------ | ---------------------- |
| Max Trial Count  | 100 (50*2)   | Usage limitation       |
| Image Page Index | 0/1          | Multi-document support |
| Date Format      | yyyy-mm-dd   | ISO 8601 compliance    |
| License Check    | 10s Interval | Real-time validation   |

------

### **7. Code Structure**

#### **7.1 Critical Functions**

```
OnProcessProc()       // Main processing entry
sendPostRequest()     // Backend service proxy
replaceAll()          // String sanitization
```

#### **7.2 Class Hierarchy**

```
ClaHTTPServerWrapper
└── MyRequestHandler
    ├── OnProcessProc
    ├── OnStatus
    └── OnNoLicense
```

------

### **8. Results**

<div align="center">
   <img src=https://github.com/LucaIT523/C_IDCardSDK_AI_WebServer/blob/main/images/1.png>
</div>



<div align="center">
   <img src=https://github.com/LucaIT523/C_IDCardSDK_AI_WebServer/blob/main/images/2.png>
</div>



<div align="center">
   <img src=https://github.com/LucaIT523/C_IDCardSDK_AI_WebServer/blob/main/images/3.png>
</div>







### **Contact Us**

For any inquiries or questions, please contact us.

telegram : @topdev1012

email :  skymorning523@gmail.com

Teams :  https://teams.live.com/l/invite/FEA2FDDFSy11sfuegI