#pragma once

#include <cstring>
#include <string>
#include <vector>

class DynaArray
{
public:
	DynaArray() { size_ = 0;  data_ = nullptr; };
	DynaArray(const DynaArray& value) {
		CopyFrom(value);
	};
	DynaArray(const std::string& value) {
		CopyFrom(value);
	};
	DynaArray(const std::vector<unsigned char>& value) {
		CopyFrom(value);
	};
	DynaArray(const char* value) {
		CopyFrom(value);
	};
	~DynaArray() { data_ = nullptr; size_ = 0; };
	DynaArray& operator=(const DynaArray& value) {
		CopyFrom(value);
		return *this;
	};
	bool operator==(const DynaArray& value) const {
		return this->size() == value.size() && (this->size() == 0 || strcmp(this->data(), value.data()) == 0);
	};
	bool operator!=(const DynaArray& value) const {
		return ! operator==(value);
	}

	std::string str() const { return std::string(data(), size()); };
	const char* data() const { return (size_==0?"":data_.get()); };
	size_t size() const { return size_; };

	//template<typename T>
	//DynaArray operator+(const T& value) {
	//	DynaArray _v = *this;
	//	_v.Append(value);
	//	return _v;
	//};

	template<typename T>
	friend DynaArray operator+(const DynaArray& value0, const T& value1) {
		DynaArray _v = value0;
		_v.Append(value1);
		return _v;
	};
private:
	template<typename T>
	void CopyFrom(const T& value)
	{
		data_.reset();
		size_ = 0;
		Append(value);
	};
	template<typename T>
	void Append(const T& value)
	{
		size_t _end_pos = size_;
		size_ += value.size();
		if (size_ > 0) {
			std::shared_ptr<char> _old_data = data_;
			data_ = std::shared_ptr<char>(new char[size_ + 1]);
			data_.get()[size_] = '\0';
			if (_end_pos > 0)
				memcpy(data_.get(), _old_data.get(), _end_pos);
			memcpy(data_.get() + _end_pos, value.data(), size_ - _end_pos);
		}
	};
	void Append(const char* value)
	{
		size_t _end_pos = size_;
		size_ += strlen(value);
		if (size_ > 0) {
			std::shared_ptr<char> _old_data = data_;
			data_ = std::shared_ptr<char>(new char[size_+1]);
			data_.get()[size_] = '\0';
			if (_end_pos > 0)
				memcpy(data_.get(), _old_data.get(), _end_pos);
			memcpy(data_.get() + _end_pos, value, size_ - _end_pos);
		}
	};
	std::shared_ptr<char> data_;
	size_t size_;
};


inline bool operator<(const DynaArray& v1, const DynaArray& v2) {
	return strcmp(v1.data(), v2.data()) < 0;
}