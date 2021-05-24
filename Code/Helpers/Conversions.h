#pragma once

 template<typename T>
 const char* EnumToString(T value)
 {
 	auto& desc = yasli::getEnumDescription<T>();
 	return desc.label(static_cast<int>(value));
 }