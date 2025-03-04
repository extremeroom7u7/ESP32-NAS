// File upload handler
void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  switch (upload.status) {
    case UPLOAD_FILE_START: {
      // Determine upload path (use current directory or root)
      String uploadPath = currentUploadPath;
      if (!uploadPath.endsWith("/")) uploadPath += "/";
      uploadPath += upload.filename;
      
      // Ensure directory exists
      String directory = uploadPath.substring(0, uploadPath.lastIndexOf('/'));
      if (!SD.exists(directory)) {
        SD.mkdir(directory);
      }
      
      // Open file for writing
      uploadFile = SD.open(uploadPath, FILE_WRITE);
      
      if (!uploadFile) {
        Serial.println("Failed to open file for writing");
      }
      
      isUploading = true;
      break;
    }
    
    case UPLOAD_FILE_WRITE: {
      if (uploadFile) {
        uploadFile.write(upload.buf, upload.currentSize);
      }
      break;
    }
    
    case UPLOAD_FILE_END: {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("Upload Complete: %s\n", upload.filename.c_str());
      }
      isUploading = false;
      break;
    }
    
    case UPLOAD_FILE_ABORTED: {
      if (uploadFile) {
        uploadFile.close();
        SD.remove(currentUploadPath + "/" + upload.filename);
      }
      isUploading = false;
      break;
    }
  }
}

// File list handler
void handleFileList() {
  // Get current directory from query parameter
  String path = server.hasArg("path") ? server.arg("path") : "/";
  
  // Ensure path starts with /
  if (!path.startsWith("/")) path = "/" + path;
  
  // Update current upload path
  currentUploadPath = path;
  
  // Create JSON document
  DynamicJsonDocument jsonDoc(4096);
  JsonArray filesArray = jsonDoc.createNestedArray("files");
  JsonArray dirsArray = jsonDoc.createNestedArray("directories");
  
  // Open directory
  File dir = SD.open(path);
  if (!dir) {
    server.send(500, "text/plain", "Failed to open directory");
    return;
  }
  
  // Iterate through directory contents
  while (File entry = dir.openNextFile()) {
    // Skip hidden files
    if (entry.name()[0] == '.') {
      entry.close();
      continue;
    }
    
    String entryPath = path + (path.endsWith("/") ? "" : "/") + entry.name();
    
    if (entry.isDirectory()) {
      // Add directory to directories array
      JsonObject dirObj = dirsArray.createNestedObject();
      dirObj["name"] = entry.name();
      dirObj["path"] = entryPath;
    } else {
      // Add file to files array
      JsonObject fileObj = filesArray.createNestedObject();
      fileObj["name"] = entry.name();
      fileObj["path"] = entryPath;
      fileObj["size"] = entry.size();
    }
    
    entry.close();
  }
  
  dir.close();
  
  // Serialize JSON
  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);
  
  // Send response
  server.send(200, "application/json", jsonResponse);
}

// File delete handler
void handleFileDelete() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Path parameter missing");
    return;
  }
  
  String path = server.arg("path");
  
  // Check if file or directory exists
  if (SD.exists(path)) {
    // Attempt to remove
    bool success = SD.remove(path);
    
    if (success) {
      server.send(200, "text/plain", "Deleted successfully");
    } else {
      server.send(500, "text/plain", "Delete failed");
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

// File download handler
void handleFileDownload() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Path parameter missing");
    return;
  }
  
  String path = server.arg("path");
  
  if (SD.exists(path)) {
    File downloadFile = SD.open(path, FILE_READ);
    
    // Extract filename
    String filename = path.substring(path.lastIndexOf('/') + 1);
    
    server.sendHeader("Content-Type", "application/octet-stream");
    server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
    server.sendHeader("Connection", "close");
    
    server.streamFile(downloadFile, "application/octet-stream");
    
    downloadFile.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

// Create directory handler
void handleCreateDirectory() {
  if (!server.hasArg("dir")) {
    server.send(400, "text/plain", "Directory name missing");
    return;
  }
  
  String dirPath = currentUploadPath + "/" + server.arg("dir");
  
  // Ensure no duplicate slashes
  dirPath.replace("//", "/");
  
  if (SD.mkdir(dirPath)) {
    server.send(200, "text/plain", "Directory created successfully");
  } else {
    server.send(500, "text/plain", "Failed to create directory");
  }
}
