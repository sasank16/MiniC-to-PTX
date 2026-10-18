#pragma once

#include "minic/semantic/Type.hpp"
#include <string>
#include <memory>
#include <sstream>

namespace minic::ir {

enum class ValueKind {
    VirtualRegister,
    ConstantInt,
    Parameter
};

class Value {
public:
    Value(ValueKind kind, semantic::Type type)
        : kind_(kind), type_(type) {}
    virtual ~Value() = default;

    ValueKind kind() const { return kind_; }
    semantic::Type type() const { return type_; }

    virtual std::string toString() const = 0;

private:
    ValueKind kind_;
    semantic::Type type_;
};

class VirtualRegister : public Value {
public:
    VirtualRegister(int id, semantic::Type type, bool isPredicate = false)
        : Value(ValueKind::VirtualRegister, type), id_(id), isPredicate_(isPredicate) {}

    int id() const { return id_; }
    bool isPredicate() const { return isPredicate_; }

    std::string toString() const override {
        if (isPredicate_) {
            return "%p" + std::to_string(id_);
        }
        return "%t" + std::to_string(id_);
    }

private:
    int id_;
    bool isPredicate_{false};
};

class ConstantInt : public Value {
public:
    explicit ConstantInt(int value)
        : Value(ValueKind::ConstantInt, semantic::Type::getInt()), value_(value) {}

    int value() const { return value_; }

    std::string toString() const override {
        return std::to_string(value_);
    }

private:
    int value_;
};

class Parameter : public Value {
public:
    Parameter(std::string name, semantic::Type type)
        : Value(ValueKind::Parameter, type), name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    std::string toString() const override {
        return "%" + name_;
    }

private:
    std::string name_;
};

} // namespace minic::ir
