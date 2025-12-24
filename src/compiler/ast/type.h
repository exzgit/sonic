#pragma once

#include <string>
#include <vector>
#include <memory>

using namespace std;

namespace frontend {

  class Visitor;

    struct Type {
      virtual ~Type() = default;
      virtual void accept(Visitor& v) = 0;
    };

    enum StandardTypeKind {
      String_Type,
      Char_Type,
      Boolean_Type,

      I8_Type,
      I16_Type,
      I32_Type,
      I64_Type,
      I128_Type,

      U8_Type,
      U16_Type,
      U32_Type,
      U64_Type,
      U128_Type,

      F32_Type,
      F64_Type,
    };

    inline string standardTypeKindToString(StandardTypeKind kind) {
      switch (kind) {
        case StandardTypeKind::String_Type: return "string";
        case StandardTypeKind::Char_Type: return "char";
        case StandardTypeKind::Boolean_Type: return "bool";
        case StandardTypeKind::I8_Type: return "i8";
        case StandardTypeKind::I16_Type: return "i16";
        case StandardTypeKind::I32_Type: return "i32";
        case StandardTypeKind::I64_Type: return "i64";
        case StandardTypeKind::I128_Type: return "i128";
        case StandardTypeKind::U8_Type: return "u8";
        case StandardTypeKind::U16_Type: return "u16";
        case StandardTypeKind::U32_Type: return "u32";
        case StandardTypeKind::U64_Type: return "u64";
        case StandardTypeKind::U128_Type: return "u128";
        case StandardTypeKind::F32_Type: return "f32";
        case StandardTypeKind::F64_Type: return "f64";
        default: return "unknown type";
      }
    }

    struct AutoType : Type {
      AutoType() = default;

      void accept(Visitor& v) override;
    };

    struct VoidType : Type {
      VoidType() = default;

      void accept(Visitor& v) override;
    };

    struct AnyType : Type {
      AnyType() = default;

      void accept(Visitor& v) override;
    };
    
    struct StandardType : Type {
      StandardTypeKind kinds;
      StandardType(StandardTypeKind k) : kinds(k) {}

      void accept(Visitor& v) override;
    };

    struct GenericType : Type {
      unique_ptr<Type> base;
      vector<unique_ptr<Type>> types;

      GenericType(unique_ptr<Type> base,
        vector<unique_ptr<Type>> types)
        : base(std::move(base)), types(std::move(types)) {}

      void accept(Visitor& v) override;
    };

    struct QualifiedType : Type {
      vector<string> segments;

      QualifiedType(vector<string> seg) : segments(std::move(seg)) {}

      void accept(Visitor& v) override;
    };

    struct PointerType : Type {
      unique_ptr<Type> base;

      PointerType(unique_ptr<Type> base) : base(std::move(base)) {}

      void accept(Visitor& v) override;
    };

    struct ReferenceType : Type {
      unique_ptr<Type> base;

      ReferenceType(unique_ptr<Type> base) : base(std::move(base)) {}

      void accept(Visitor& v) override;
    };
};
