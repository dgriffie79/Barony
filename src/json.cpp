#include "main.hpp"
#include "files.hpp"
#include "json.hpp"
#include "cJSON.h"

#include <cassert>
#include <vector>

const Uint32 BinaryFormatTag = *"spff";

struct WriterStackEntry {
	cJSON* obj;
	const char* name;
};

class JsonFileWriter : public FileInterface {
public:
	JsonFileWriter(EFileFormat format)
	: root(nullptr)
	, current(nullptr)
	, currentName(nullptr)
	, compact(format == EFileFormat::Json_Compact)
	{
	}

	~JsonFileWriter() {
		if (root) cJSON_Delete(root);
	}

	static bool writeObject(File* file, const FileHelper::SerializationFunc& serialize, EFileFormat format) {
		JsonFileWriter jfw(format);

		bool result = serialize(&jfw);

		jfw.save(file);
		return result;
	}

	virtual bool isReading() const override { return false; }

	virtual bool beginObject() override {
		cJSON* obj = cJSON_CreateObject();
		if (!root) {
			root = obj;
			current = obj;
		} else {
			stack.push_back({current, currentName});
			current = obj;
		}
		currentName = nullptr;
		return true;
	}

	virtual void endObject() override {
		if (stack.empty()) {
			return;
		}
		cJSON* parent = stack.back().obj;
		const char* name = stack.back().name;
		stack.pop_back();
		if (name) {
			cJSON_AddItemToObject(parent, name, current);
		} else if (cJSON_IsArray(parent)) {
			cJSON_AddItemToArray(parent, current);
		}
		current = parent;
		currentName = nullptr;
	}

	virtual bool beginArray(Uint32 & size) override {
		cJSON* arr = cJSON_CreateArray();
		if (!root) {
			root = arr;
			current = arr;
		} else {
			stack.push_back({current, currentName});
			current = arr;
		}
		currentName = nullptr;
		return true;
	}

	virtual void endArray() override {
		if (stack.empty()) return;
		cJSON* parent = stack.back().obj;
		const char* name = stack.back().name;
		stack.pop_back();
		if (name) {
			cJSON_AddItemToObject(parent, name, current);
		}
		current = parent;
		currentName = nullptr;
	}

	virtual void propertyName(const char * fieldName) override {
		currentName = fieldName;
	}

	virtual bool value(Uint32& value) override {
		return addItem(cJSON_CreateNumber((double)value));
	}
	virtual bool value(Sint32& value) override {
		return addItem(cJSON_CreateNumber((double)value));
	}
	virtual bool value(float& value) override {
		return addItem(cJSON_CreateNumber((double)value));
	}
	virtual bool value(double& value) override {
		return addItem(cJSON_CreateNumber(value));
	}
	virtual bool value(bool& value) override {
		return addItem(cJSON_CreateBool((cJSON_bool)value));
	}
	virtual bool value(std::string& value) override {
		return addItem(cJSON_CreateString(value.c_str()));
	}

private:

	bool addItem(cJSON* item) {
		if (!item || !current) return false;
		if (currentName) {
			cJSON_AddItemToObject(current, currentName, item);
			currentName = nullptr;
		} else if (cJSON_IsArray(current)) {
			cJSON_AddItemToArray(current, item);
		} else {
			cJSON_Delete(item);
			return false;
		}
		return true;
	}

	void save(File* file) {
		if (!root) return;
		char* json = compact ? cJSON_PrintUnformatted(root) : cJSON_Print(root);
		if (json) {
			file->puts(json);
			cJSON_free(json);
		}
	}

	cJSON* root;
	cJSON* current;
	std::vector<WriterStackEntry> stack;
	const char* currentName = nullptr;
	bool compact;
};

struct ReaderStackEntry {
	cJSON* obj;
	int arrayIndex;
};

class JsonFileReader : public FileInterface {
public:
	~JsonFileReader() {
		if (doc) cJSON_Delete(doc);
	}

	static bool readObject(File * fp, const FileHelper::SerializationFunc & serialize) {
		JsonFileReader jfr;

		if (!jfr.readAllFileData(fp)) {
			return false;
		}

		bool result = serialize(&jfr);

		return result;
	}

	virtual bool isReading() const override { return true; }

	virtual bool beginObject() override {
		if (stack.empty()) {
			if (doc && cJSON_IsObject(doc)) {
				stack.push_back({doc, -1});
				return true;
			}
			return false;
		}
		cJSON* child = getCurrentChild();
		if (child && cJSON_IsObject(child)) {
			stack.push_back({child, -1});
			return true;
		}
		return false;
	}

	virtual void endObject() override {
		if (!stack.empty()) {
			stack.pop_back();
		}
	}

	virtual bool beginArray(Uint32 & size) override {
		cJSON* arr = getCurrentChild();
		if (arr && cJSON_IsArray(arr)) {
			stack.push_back({arr, 0});
			size = cJSON_GetArraySize(arr);
			return true;
		}
		return false;
	}

	virtual void endArray() override {
		if (!stack.empty()) {
			stack.pop_back();
		}
	}

	virtual void propertyName(const char * fieldName) override {
		propName = fieldName;
	}

	virtual bool value(Uint32& value) override {
		cJSON* item = getCurrentChild();
		if (item && cJSON_IsNumber(item)) {
			value = (Uint32)item->valuedouble;
			return true;
		}
		return false;
	}

	virtual bool value(Sint32& value) override {
		cJSON* item = getCurrentChild();
		if (item && cJSON_IsNumber(item)) {
			value = (Sint32)item->valuedouble;
			return true;
		}
		return false;
	}

	virtual bool value(float& value) override {
		cJSON* item = getCurrentChild();
		if (item && cJSON_IsNumber(item)) {
			value = (float)item->valuedouble;
			return true;
		}
		return false;
	}

	virtual bool value(double& value) override {
		cJSON* item = getCurrentChild();
		if (item && cJSON_IsNumber(item)) {
			value = item->valuedouble;
			return true;
		}
		return false;
	}

	virtual bool value(bool& value) override {
		cJSON* item = getCurrentChild();
		if (item && cJSON_IsBool(item)) {
			value = cJSON_IsTrue(item) ? true : false;
			return true;
		}
		return false;
	}

	virtual bool value(std::string& value) override {
		cJSON* item = getCurrentChild();
		if (item && cJSON_IsString(item)) {
			value = item->valuestring;
			return true;
		}
		return false;
	}

protected:

	cJSON* getCurrentChild() {
		if (stack.empty()) {
			if (propName && doc) {
				cJSON* result = cJSON_GetObjectItem(doc, propName);
				propName = nullptr;
				return result;
			}
			return doc;
		}

		ReaderStackEntry& top = stack.back();
		if (cJSON_IsArray(top.obj)) {
			if (top.arrayIndex >= 0) {
				return cJSON_GetArrayItem(top.obj, top.arrayIndex++);
			}
			return nullptr;
		}

		if (propName) {
			cJSON* result = cJSON_GetObjectItem(top.obj, propName);
			propName = nullptr;
			return result;
		}

		return top.obj;
	}

	bool readAllFileData(File * fp) {
		long size = fp->size();
		char * data = (char *)calloc(sizeof(char), size + 1);
		assert(data);

		size_t bytesRead = fp->read(data, sizeof(char), size);
		if (bytesRead != size) {
			printlog("JsonFileReader: failed to read data (%d)", errno);
			free(data);
			return false;
		}

		data[size] = 0;
		doc = cJSON_Parse(data);
		free(data);

		if (!doc) {
			printlog("JsonFileReader: parse error: %s", cJSON_GetErrorPtr());
			return false;
		}

		return true;
	}

	cJSON* doc = nullptr;
	const char* propName = nullptr;
	std::vector<ReaderStackEntry> stack;
};

class BinaryFileWriter : public FileInterface {
public:

	BinaryFileWriter(File * file)
	: fp(file)
	{
	}

	~BinaryFileWriter() {
	}

	static bool writeObject(File * fp, const FileHelper::SerializationFunc & serialize) {
		BinaryFileWriter bfw(fp);

		bfw.writeHeader();

		if (bfw.beginObject()) {
			bool result = serialize(&bfw);
			bfw.endObject();
			return result;
		} else {
			return false;
		}
	}

	virtual bool isReading() const override { return false; }

	virtual bool beginObject() override {
		return true;
	}

	virtual void endObject() override {
	}

	virtual bool beginArray(Uint32 & size) override {
		return fp->write(&size, sizeof(size), 1) == 1;
	}

	virtual void endArray() override {
	}

	virtual void propertyName(const char * name) override {
	}

	virtual bool value(Uint32& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(Sint32& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(float& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(double& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(bool& v) override {
		return fp->write(&v, sizeof(v), 1) == 1;
	}
	virtual bool value(std::string& v) override {
		return writeStringInternal(v);
	}

private:

	void writeHeader() {
		(void)fp->write(&BinaryFormatTag, sizeof(BinaryFormatTag), 1);
	}

	bool writeStringInternal(const std::string& v) {
		Uint32 len = (Uint32)v.size();
		bool result = true;
		result = fp->write(&len, sizeof(len), 1) == 1 ? result : false;
		if (len) {
			result = fp->write(v.c_str(), sizeof(char), len) == len ?
				result : false;
		}
		return result;
	}

	File* fp = nullptr;
};

class BinaryFileReader : public FileInterface {
public:

	BinaryFileReader(File * file)
		: fp(file)
	{
	}

	static bool readObject(File * fp, const FileHelper::SerializationFunc & serialize) {
		BinaryFileReader bfr(fp);

		if (!bfr.readHeader()) {
			return false;
		}

		bfr.beginObject();
		bool result = serialize(&bfr);
		bfr.endObject();

		return result;
	}

	virtual bool isReading() const override { return true; }

	virtual bool beginObject() override {
		return true;
	}

	virtual void endObject() override {
	}

	virtual bool beginArray(Uint32 & size) override {
		return fp->read(&size, sizeof(size), 1) == 1;
	}

	virtual void endArray() override {
	}

	virtual void propertyName(const char * name) override {
	}

	virtual bool value(Uint32& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(Sint32& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(float& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(double& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(bool& v) override {
		size_t read = fp->read(&v, sizeof(v), 1);
		return read == 1;
	}
	virtual bool value(std::string& v) override {
		bool result = readStringInternal(v);
		return result;
	}

private:

	bool readHeader() {
		Uint32 fileFormatTag;
		size_t read = fp->read(&fileFormatTag, sizeof(fileFormatTag), 1);
		if (read != 1) {
			printlog("BinaryFileReader: failed to read format tag (%d)", errno);
			return false;
		}

		if (fileFormatTag != BinaryFormatTag) {
			printlog("BinaryFileReader: file format tag mismatch (expected %x, got %x)", BinaryFormatTag, fileFormatTag);
			return false;
		}

		return true;
	}

	bool readStringInternal(std::string & v) {
		Uint32 len;
		bool result = true;
		size_t read = fp->read(&len, sizeof(len), 1);
		result = read == 1 ? result : false;

		if (len) {
			v.reserve(len);
			read = fp->read(&v[0u], sizeof(char), len);
			result = read == len ? result : false;
		}

		return result;
	}

	File* fp;
};

static EFileFormat GetFileFormat(File * file) {
	Uint32 fileFormatTag = 0;
	file->read(&fileFormatTag, sizeof(fileFormatTag), 1);
	file->seek(0, FileBase::SeekMode::SET);

	if (fileFormatTag == BinaryFormatTag) {
		return EFileFormat::Binary;
	}
	else {
		return EFileFormat::Json;
	}
}

bool FileHelper::writeObjectInternal(const char * filename, EFileFormat format, const SerializationFunc& serialize) {
	File * file = FileIO::open(filename, "wb");
#ifndef NDEBUG
	printlog("Opening file '%s' for write", filename);
#endif
	if (!file) {
		printlog("Unable to open file '%s' for write (%d)", filename, errno);
		return false;
	}

	bool success = false;
	if (format == EFileFormat::Binary) {
		success = BinaryFileWriter::writeObject(file, serialize);
	}
	else if (format == EFileFormat::Json || format == EFileFormat::Json_Compact) {
		success = JsonFileWriter::writeObject(file, serialize, format);
	}
	else {
		assert(false);
	}

	FileIO::close(file);

	return success;
}

bool FileHelper::readObjectInternal(const char * filename, const SerializationFunc& serialize) {
	File * file = FileIO::open(filename, "rb");
#ifndef NDEBUG
	printlog("Opening file '%s' for read", filename);
#endif
	if (!file) {
		printlog("Unable to open file '%s' for read (%d)", filename, errno);
		return false;
	}

	EFileFormat format = GetFileFormat(file);

	bool success = false;
	if (format == EFileFormat::Binary) {
		success = BinaryFileReader::readObject(file, serialize);
	}
	else if(format == EFileFormat::Json || format == EFileFormat::Json_Compact) {
		success = JsonFileReader::readObject(file, serialize);
	}
	else {
		assert(false);
	}

	FileIO::close(file);

	return success;
}
