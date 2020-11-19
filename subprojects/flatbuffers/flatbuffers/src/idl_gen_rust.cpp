/*
 * Copyright 2018 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// independent from idl_parser, since this code is not needed for most clients

#include "flatbuffers/code_generators.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace flatbuffers {

// Convert a camelCaseIdentifier or CamelCaseIdentifier to a
// snake_case_indentifier.
std::string MakeSnakeCase(const std::string &in) {
  std::string s;
  for (size_t i = 0; i < in.length(); i++) {
    if (i == 0) {
      s += CharToLower(in[0]);
    } else if (in[i] == '_') {
      s += '_';
    } else if (!islower(in[i])) {
      // Prevent duplicate underscores for Upper_Snake_Case strings
      // and UPPERCASE strings.
      if (islower(in[i - 1])) { s += '_'; }
      s += CharToLower(in[i]);
    } else {
      s += in[i];
    }
  }
  return s;
}

// Convert a string to all uppercase.
std::string MakeUpper(const std::string &in) {
  std::string s;
  for (size_t i = 0; i < in.length(); i++) { s += CharToUpper(in[i]); }
  return s;
}

// Encapsulate all logical field types in this enum. This allows us to write
// field logic based on type switches, instead of branches on the properties
// set on the Type.
// TODO(rw): for backwards compatibility, we can't use a strict `enum class`
//           declaration here. could we use the `-Wswitch-enum` warning to
//           achieve the same effect?
enum FullType {
  ftInteger = 0,
  ftFloat = 1,
  ftBool = 2,

  ftStruct = 3,
  ftTable = 4,

  ftEnumKey = 5,
  ftUnionKey = 6,

  ftUnionValue = 7,

  // TODO(rw): bytestring?
  ftString = 8,

  ftVectorOfInteger = 9,
  ftVectorOfFloat = 10,
  ftVectorOfBool = 11,
  ftVectorOfEnumKey = 12,
  ftVectorOfStruct = 13,
  ftVectorOfTable = 14,
  ftVectorOfString = 15,
  ftVectorOfUnionValue = 16,
};

// Convert a Type to a FullType (exhaustive).
FullType GetFullType(const Type &type) {
  // N.B. The order of these conditionals matters for some types.

  if (IsString(type)) {
    return ftString;
  } else if (type.base_type == BASE_TYPE_STRUCT) {
    if (type.struct_def->fixed) {
      return ftStruct;
    } else {
      return ftTable;
    }
  } else if (IsVector(type)) {
    switch (GetFullType(type.VectorType())) {
      case ftInteger: {
        return ftVectorOfInteger;
      }
      case ftFloat: {
        return ftVectorOfFloat;
      }
      case ftBool: {
        return ftVectorOfBool;
      }
      case ftStruct: {
        return ftVectorOfStruct;
      }
      case ftTable: {
        return ftVectorOfTable;
      }
      case ftString: {
        return ftVectorOfString;
      }
      case ftEnumKey: {
        return ftVectorOfEnumKey;
      }
      case ftUnionKey:
      case ftUnionValue: {
        FLATBUFFERS_ASSERT(false && "vectors of unions are unsupported");
        break;
      }
      default: {
        FLATBUFFERS_ASSERT(false && "vector of vectors are unsupported");
      }
    }
  } else if (type.enum_def != nullptr) {
    if (type.enum_def->is_union) {
      if (type.base_type == BASE_TYPE_UNION) {
        return ftUnionValue;
      } else if (IsInteger(type.base_type)) {
        return ftUnionKey;
      } else {
        FLATBUFFERS_ASSERT(false && "unknown union field type");
      }
    } else {
      return ftEnumKey;
    }
  } else if (IsScalar(type.base_type)) {
    if (IsBool(type.base_type)) {
      return ftBool;
    } else if (IsInteger(type.base_type)) {
      return ftInteger;
    } else if (IsFloat(type.base_type)) {
      return ftFloat;
    } else {
      FLATBUFFERS_ASSERT(false && "unknown number type");
    }
  }

  FLATBUFFERS_ASSERT(false && "completely unknown type");

  // this is only to satisfy the compiler's return analysis.
  return ftBool;
}

// If the second parameter is false then wrap the first with Option<...>
std::string WrapInOptionIfNotRequired(std::string s, bool required) {
  if (required) {
    return s;
  } else {
    return "Option<" + s + ">";
  }
}

// If the second parameter is false then add .unwrap()
std::string AddUnwrapIfRequired(std::string s, bool required) {
  if (required) {
    return s + ".unwrap()";
  } else {
    return s;
  }
}

bool IsBitFlagsEnum(const EnumDef &enum_def) {
  return enum_def.attributes.Lookup("bit_flags") != nullptr;
}
bool IsBitFlagsEnum(const FieldDef &field) {
  EnumDef* ed = field.value.type.enum_def;
  return ed && IsBitFlagsEnum(*ed);
}

namespace rust {

class RustGenerator : public BaseGenerator {
 public:
  RustGenerator(const Parser &parser, const std::string &path,
                const std::string &file_name)
      : BaseGenerator(parser, path, file_name, "", "::", "rs"),
        cur_name_space_(nullptr) {
    const char *keywords[] = {
      // list taken from:
      // https://doc.rust-lang.org/book/second-edition/appendix-01-keywords.html
      //
      // we write keywords one per line so that we can easily compare them with
      // changes to that webpage in the future.

      // currently-used keywords
      "as", "break", "const", "continue", "crate", "else", "enum", "extern",
      "false", "fn", "for", "if", "impl", "in", "let", "loop", "match", "mod",
      "move", "mut", "pub", "ref", "return", "Self", "self", "static", "struct",
      "super", "trait", "true", "type", "unsafe", "use", "where", "while",

      // future possible keywords
      "abstract", "alignof", "become", "box", "do", "final", "macro",
      "offsetof", "override", "priv", "proc", "pure", "sizeof", "typeof",
      "unsized", "virtual", "yield",

      // other rust terms we should not use
      "std", "usize", "isize", "u8", "i8", "u16", "i16", "u32", "i32", "u64",
      "i64", "u128", "i128", "f32", "f64",

      // These are terms the code generator can implement on types.
      //
      // In Rust, the trait resolution rules (as described at
      // https://github.com/rust-lang/rust/issues/26007) mean that, as long
      // as we impl table accessors as inherent methods, we'll never create
      // conflicts with these keywords. However, that's a fairly nuanced
      // implementation detail, and how we implement methods could change in
      // the future. as a result, we proactively block these out as reserved
      // words.
      "follow", "push", "size", "alignment", "to_little_endian",
      "from_little_endian", nullptr,

      // used by Enum constants
      "ENUM_MAX", "ENUM_MIN", "ENUM_VALUES",
    };
    for (auto kw = keywords; *kw; kw++) keywords_.insert(*kw);
  }

  // Iterate through all definitions we haven't generated code for (enums,
  // structs, and tables) and output them to a single file.
  bool generate() {
    code_.Clear();
    code_ += "// " + std::string(FlatBuffersGeneratedWarning()) + "\n\n";

    assert(!cur_name_space_);

    // Generate imports for the global scope in case no namespace is used
    // in the schema file.
    GenNamespaceImports(0);
    code_ += "";

    // Generate all code in their namespaces, once, because Rust does not
    // permit re-opening modules.
    //
    // TODO(rw): Use a set data structure to reduce namespace evaluations from
    //           O(n**2) to O(n).
    for (auto ns_it = parser_.namespaces_.begin();
         ns_it != parser_.namespaces_.end(); ++ns_it) {
      const auto &ns = *ns_it;

      // Generate code for all the enum declarations.
      for (auto it = parser_.enums_.vec.begin(); it != parser_.enums_.vec.end();
           ++it) {
        const auto &enum_def = **it;
        if (enum_def.defined_namespace != ns) { continue; }
        if (!enum_def.generated) {
          SetNameSpace(enum_def.defined_namespace);
          GenEnum(enum_def);
        }
      }

      // Generate code for all structs.
      for (auto it = parser_.structs_.vec.begin();
           it != parser_.structs_.vec.end(); ++it) {
        const auto &struct_def = **it;
        if (struct_def.defined_namespace != ns) { continue; }
        if (struct_def.fixed && !struct_def.generated) {
          SetNameSpace(struct_def.defined_namespace);
          GenStruct(struct_def);
        }
      }

      // Generate code for all tables.
      for (auto it = parser_.structs_.vec.begin();
           it != parser_.structs_.vec.end(); ++it) {
        const auto &struct_def = **it;
        if (struct_def.defined_namespace != ns) { continue; }
        if (!struct_def.fixed && !struct_def.generated) {
          SetNameSpace(struct_def.defined_namespace);
          GenTable(struct_def);
        }
      }

      // Generate global helper functions.
      if (parser_.root_struct_def_) {
        auto &struct_def = *parser_.root_struct_def_;
        if (struct_def.defined_namespace != ns) { continue; }
        SetNameSpace(struct_def.defined_namespace);
        GenRootTableFuncs(struct_def);
      }
    }
    if (cur_name_space_) SetNameSpace(nullptr);

    const auto file_path = GeneratedFileName(path_, file_name_, parser_.opts);
    const auto final_code = code_.ToString();
    return SaveFile(file_path.c_str(), final_code, false);
  }

 private:
  CodeWriter code_;

  std::set<std::string> keywords_;

  // This tracks the current namespace so we can insert namespace declarations.
  const Namespace *cur_name_space_;

  const Namespace *CurrentNameSpace() const { return cur_name_space_; }

  // Determine if a Type needs a lifetime template parameter when used in the
  // Rust builder args.
  bool TableBuilderTypeNeedsLifetime(const Type &type) const {
    switch (GetFullType(type)) {
      case ftInteger:
      case ftFloat:
      case ftBool:
      case ftEnumKey:
      case ftUnionKey:
      case ftUnionValue: {
        return false;
      }
      default: {
        return true;
      }
    }
  }

  // Determine if a table args rust type needs a lifetime template parameter.
  bool TableBuilderArgsNeedsLifetime(const StructDef &struct_def) const {
    FLATBUFFERS_ASSERT(!struct_def.fixed);

    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      if (field.deprecated) { continue; }

      if (TableBuilderTypeNeedsLifetime(field.value.type)) { return true; }
    }

    return false;
  }

  std::string EscapeKeyword(const std::string &name) const {
    return keywords_.find(name) == keywords_.end() ? name : name + "_";
  }

  std::string Name(const Definition &def) const {
    return EscapeKeyword(def.name);
  }

  std::string Name(const EnumVal &ev) const { return EscapeKeyword(ev.name); }

  std::string WrapInNameSpace(const Definition &def) const {
    return WrapInNameSpace(def.defined_namespace, Name(def));
  }
  std::string WrapInNameSpace(const Namespace *ns,
                              const std::string &name) const {
    if (CurrentNameSpace() == ns) return name;
    std::string prefix = GetRelativeNamespaceTraversal(CurrentNameSpace(), ns);
    return prefix + name;
  }

  // Determine the namespace traversal needed from the Rust crate root.
  // This may be useful in the future for referring to included files, but is
  // currently unused.
  std::string GetAbsoluteNamespaceTraversal(const Namespace *dst) const {
    std::stringstream stream;

    stream << "::";
    for (auto d = dst->components.begin(); d != dst->components.end(); ++d) {
      stream << MakeSnakeCase(*d) + "::";
    }
    return stream.str();
  }

  // Determine the relative namespace traversal needed to reference one
  // namespace from another namespace. This is useful because it does not force
  // the user to have a particular file layout. (If we output absolute
  // namespace paths, that may require users to organize their Rust crates in a
  // particular way.)
  std::string GetRelativeNamespaceTraversal(const Namespace *src,
                                            const Namespace *dst) const {
    // calculate the path needed to reference dst from src.
    // example: f(A::B::C, A::B::C) -> (none)
    // example: f(A::B::C, A::B)    -> super::
    // example: f(A::B::C, A::B::D) -> super::D
    // example: f(A::B::C, A)       -> super::super::
    // example: f(A::B::C, D)       -> super::super::super::D
    // example: f(A::B::C, D::E)    -> super::super::super::D::E
    // example: f(A, D::E)          -> super::D::E
    // does not include leaf object (typically a struct type).

    size_t i = 0;
    std::stringstream stream;

    auto s = src->components.begin();
    auto d = dst->components.begin();
    for (;;) {
      if (s == src->components.end()) { break; }
      if (d == dst->components.end()) { break; }
      if (*s != *d) { break; }
      ++s;
      ++d;
      ++i;
    }

    for (; s != src->components.end(); ++s) { stream << "super::"; }
    for (; d != dst->components.end(); ++d) {
      stream << MakeSnakeCase(*d) + "::";
    }
    return stream.str();
  }

  // Generate a comment from the schema.
  void GenComment(const std::vector<std::string> &dc, const char *prefix = "") {
    std::string text;
    ::flatbuffers::GenComment(dc, &text, nullptr, prefix);
    code_ += text + "\\";
  }

  // Return a Rust type from the table in idl.h.
  std::string GetTypeBasic(const Type &type) const {
    switch (GetFullType(type)) {
      case ftInteger:
      case ftFloat:
      case ftBool:
      case ftEnumKey:
      case ftUnionKey: {
        break;
      }
      default: {
        FLATBUFFERS_ASSERT(false && "incorrect type given");
      }
    }

    // clang-format off
    static const char * const ctypename[] = {
    #define FLATBUFFERS_TD(ENUM, IDLTYPE, CTYPE, JTYPE, GTYPE, NTYPE, PTYPE, \
                           RTYPE, ...) \
      #RTYPE,
      FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
    #undef FLATBUFFERS_TD
    };
    // clang-format on

    if (type.enum_def) { return WrapInNameSpace(*type.enum_def); }
    return ctypename[type.base_type];
  }

  // Look up the native type for an enum. This will always be an integer like
  // u8, i32, etc.
  std::string GetEnumTypeForDecl(const Type &type) {
    const auto ft = GetFullType(type);
    if (!(ft == ftEnumKey || ft == ftUnionKey)) {
      FLATBUFFERS_ASSERT(false && "precondition failed in GetEnumTypeForDecl");
    }

    // clang-format off
    static const char *ctypename[] = {
    #define FLATBUFFERS_TD(ENUM, IDLTYPE, CTYPE, JTYPE, GTYPE, NTYPE, PTYPE, \
                           RTYPE, ...) \
      #RTYPE,
      FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
    #undef FLATBUFFERS_TD
    };
    // clang-format on

    // Enums can be bools, but their Rust representation must be a u8, as used
    // in the repr attribute (#[repr(bool)] is an invalid attribute).
    if (type.base_type == BASE_TYPE_BOOL) return "u8";
    return ctypename[type.base_type];
  }

  // Return a Rust type for any type (scalar, table, struct) specifically for
  // using a FlatBuffer.
  std::string GetTypeGet(const Type &type) const {
    switch (GetFullType(type)) {
      case ftInteger:
      case ftFloat:
      case ftBool:
      case ftEnumKey:
      case ftUnionKey: {
        return GetTypeBasic(type);
      }
      case ftTable: {
        return WrapInNameSpace(type.struct_def->defined_namespace,
                               type.struct_def->name) +
               "<'a>";
      }
      default: {
        return WrapInNameSpace(type.struct_def->defined_namespace,
                               type.struct_def->name);
      }
    }
  }

  std::string GetEnumValue(const EnumDef &enum_def,
                            const EnumVal &enum_val) const {
    return Name(enum_def) + "::" + Name(enum_val);
  }

  // 1 suffix since old C++ can't figure out the overload.
  void ForAllEnumValues1(const EnumDef &enum_def,
                        std::function<void(const EnumVal&)> cb) {
    for (auto it = enum_def.Vals().begin(); it != enum_def.Vals().end(); ++it) {
      const auto &ev = **it;
      code_.SetValue("VARIANT", Name(ev));
      code_.SetValue("VALUE", enum_def.ToString(ev));
      cb(ev);
    }
  }
  void ForAllEnumValues(const EnumDef &enum_def, std::function<void()> cb) {
      std::function<void(const EnumVal&)> wrapped = [&](const EnumVal& unused) {
        (void) unused;
        cb();
      };
      ForAllEnumValues1(enum_def, wrapped);
  }
  // Generate an enum declaration,
  // an enum string lookup table,
  // an enum match function,
  // and an enum array of values
  void GenEnum(const EnumDef &enum_def) {
    code_.SetValue("ENUM_NAME", Name(enum_def));
    code_.SetValue("BASE_TYPE", GetEnumTypeForDecl(enum_def.underlying_type));
    code_.SetValue("ENUM_NAME_SNAKE", MakeSnakeCase(Name(enum_def)));
    code_.SetValue("ENUM_NAME_CAPS", MakeUpper(MakeSnakeCase(Name(enum_def))));
    const EnumVal *minv = enum_def.MinValue();
    const EnumVal *maxv = enum_def.MaxValue();
    FLATBUFFERS_ASSERT(minv && maxv);
    code_.SetValue("ENUM_MIN_BASE_VALUE", enum_def.ToString(*minv));
    code_.SetValue("ENUM_MAX_BASE_VALUE", enum_def.ToString(*maxv));

    if (IsBitFlagsEnum(enum_def)) {
      // Defer to the convenient and canonical bitflags crate. We declare it in a
      // module to #allow camel case constants in a smaller scope. This matches
      // Flatbuffers c-modeled enums where variants are associated constants but
      // in camel case.
      code_ += "#[allow(non_upper_case_globals)]";
      code_ += "mod bitflags_{{ENUM_NAME_SNAKE}} {";
      code_ += "  flatbuffers::bitflags::bitflags! {";
      GenComment(enum_def.doc_comment, "    ");
      code_ += "    pub struct {{ENUM_NAME}}: {{BASE_TYPE}} {";
      ForAllEnumValues1(enum_def, [&](const EnumVal &ev){
        this->GenComment(ev.doc_comment, "      ");
        code_ += "      const {{VARIANT}} = {{VALUE}};";
      });
      code_ += "    }";
      code_ += "  }";
      code_ += "}";
      code_ += "pub use self::bitflags_{{ENUM_NAME_SNAKE}}::{{ENUM_NAME}};";
      code_ += "";

      // Generate Follow and Push so we can serialize and stuff.
      code_ += "impl<'a> flatbuffers::Follow<'a> for {{ENUM_NAME}} {";
      code_ += "  type Inner = Self;";
      code_ += "  #[inline]";
      code_ += "  fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {";
      code_ += "    let bits = flatbuffers::read_scalar_at::<{{BASE_TYPE}}>(buf, loc);";
      code_ += "    unsafe { Self::from_bits_unchecked(bits) }";
      code_ += "  }";
      code_ += "}";
      code_ += "";
      code_ += "impl flatbuffers::Push for {{ENUM_NAME}} {";
      code_ += "    type Output = {{ENUM_NAME}};";
      code_ += "    #[inline]";
      code_ += "    fn push(&self, dst: &mut [u8], _rest: &[u8]) {";
      code_ += "        flatbuffers::emplace_scalar::<{{BASE_TYPE}}>"
               "(dst, self.bits());";
      code_ += "    }";
      code_ += "}";
      code_ += "";
      code_ += "impl flatbuffers::EndianScalar for {{ENUM_NAME}} {";
      code_ += "  #[inline]";
      code_ += "  fn to_little_endian(self) -> Self {";
      code_ += "    let bits = {{BASE_TYPE}}::to_le(self.bits());";
      code_ += "    unsafe { Self::from_bits_unchecked(bits) }";
      code_ += "  }";
      code_ += "  #[inline]";
      code_ += "  fn from_little_endian(self) -> Self {";
      code_ += "    let bits = {{BASE_TYPE}}::from_le(self.bits());";
      code_ += "    unsafe { Self::from_bits_unchecked(bits) }";
      code_ += "  }";
      code_ += "}";
      code_ += "";
      return;
    }

    // Deprecated associated constants;
    code_ += "#[deprecated(since = \"1.13\", note = \"Use associated constants"
             " instead. This will no longer be generated in 2021.\")]";
    code_ += "pub const ENUM_MIN_{{ENUM_NAME_CAPS}}: {{BASE_TYPE}}"
             " = {{ENUM_MIN_BASE_VALUE}};";
    code_ += "#[deprecated(since = \"1.13\", note = \"Use associated constants"
             " instead. This will no longer be generated in 2021.\")]";
    code_ += "pub const ENUM_MAX_{{ENUM_NAME_CAPS}}: {{BASE_TYPE}}"
             " = {{ENUM_MAX_BASE_VALUE}};";
    auto num_fields = NumToString(enum_def.size());
    code_ += "#[deprecated(since = \"1.13\", note = \"Use associated constants"
             " instead. This will no longer be generated in 2021.\")]";
    code_ += "#[allow(non_camel_case_types)]";
    code_ += "pub const ENUM_VALUES_{{ENUM_NAME_CAPS}}: [{{ENUM_NAME}}; " +
             num_fields + "] = [";
    ForAllEnumValues1(enum_def, [&](const EnumVal &ev){
      code_ += "  " + GetEnumValue(enum_def, ev) + ",";
    });
    code_ += "];";
    code_ += "";

    GenComment(enum_def.doc_comment);
    code_ +=
        "#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]";
    code_ += "#[repr(transparent)]";
    code_ += "pub struct {{ENUM_NAME}}(pub {{BASE_TYPE}});";
    code_ += "#[allow(non_upper_case_globals)]";
    code_ += "impl {{ENUM_NAME}} {";
    ForAllEnumValues1(enum_def, [&](const EnumVal &ev){
      this->GenComment(ev.doc_comment, "  ");
      code_ += "  pub const {{VARIANT}}: Self = Self({{VALUE}});";
    });
    code_ += "";
    // Generate Associated constants
    code_ += "  pub const ENUM_MIN: {{BASE_TYPE}} = {{ENUM_MIN_BASE_VALUE}};";
    code_ += "  pub const ENUM_MAX: {{BASE_TYPE}} = {{ENUM_MAX_BASE_VALUE}};";
    code_ += "  pub const ENUM_VALUES: &'static [Self] = &[";
    ForAllEnumValues(enum_def, [&](){
      code_ += "    Self::{{VARIANT}},";
    });
    code_ += "  ];";
    code_ += "  /// Returns the variant's name or \"\" if unknown.";
    code_ += "  pub fn variant_name(self) -> Option<&'static str> {";
    code_ += "    match self {";
    ForAllEnumValues(enum_def, [&](){
      code_ += "      Self::{{VARIANT}} => Some(\"{{VARIANT}}\"),";
    });
    code_ += "      _ => None,";
    code_ += "    }";
    code_ += "  }";
    code_ += "}";

    // Generate Debug. Unknown variants are printed like "<UNKNOWN 42>".
    code_ += "impl std::fmt::Debug for {{ENUM_NAME}} {";
    code_ += "  fn fmt(&self, f: &mut std::fmt::Formatter) ->"
             " std::fmt::Result {";
    code_ += "    if let Some(name) = self.variant_name() {";
    code_ += "      f.write_str(name)";
    code_ += "    } else {";
    code_ += "      f.write_fmt(format_args!(\"<UNKNOWN {:?}>\", self.0))";
    code_ += "    }";
    code_ += "  }";
    code_ += "}";

    // Generate Follow and Push so we can serialize and stuff.
    code_ += "impl<'a> flatbuffers::Follow<'a> for {{ENUM_NAME}} {";
    code_ += "  type Inner = Self;";
    code_ += "  #[inline]";
    code_ += "  fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {";
    code_ += "    Self(flatbuffers::read_scalar_at::<{{BASE_TYPE}}>(buf, loc))";
    code_ += "  }";
    code_ += "}";
    code_ += "";
    code_ += "impl flatbuffers::Push for {{ENUM_NAME}} {";
    code_ += "    type Output = {{ENUM_NAME}};";
    code_ += "    #[inline]";
    code_ += "    fn push(&self, dst: &mut [u8], _rest: &[u8]) {";
    code_ += "        flatbuffers::emplace_scalar::<{{BASE_TYPE}}>"
             "(dst, self.0);";
    code_ += "    }";
    code_ += "}";
    code_ += "";
    code_ += "impl flatbuffers::EndianScalar for {{ENUM_NAME}} {";
    code_ += "  #[inline]";
    code_ += "  fn to_little_endian(self) -> Self {";
    code_ += "    Self({{BASE_TYPE}}::to_le(self.0))";
    code_ += "  }";
    code_ += "  #[inline]";
    code_ += "  fn from_little_endian(self) -> Self {";
    code_ += "    Self({{BASE_TYPE}}::from_le(self.0))";
    code_ += "  }";
    code_ += "}";
    code_ += "";

    if (enum_def.is_union) {
      // Generate tyoesafe offset(s) for unions
      code_.SetValue("NAME", Name(enum_def));
      code_.SetValue("UNION_OFFSET_NAME", Name(enum_def) + "UnionTableOffset");
      code_ += "pub struct {{UNION_OFFSET_NAME}} {}";
    }
  }

  std::string GetFieldOffsetName(const FieldDef &field) {
    return "VT_" + MakeUpper(Name(field));
  }

  std::string GetDefaultScalarValue(const FieldDef &field) {
    switch (GetFullType(field.value.type)) {
      case ftInteger:
      case ftFloat: {
        return field.optional ? "None" : field.value.constant;
      }
      case ftBool: {
        return field.optional ? "None"
                              : field.value.constant == "0" ? "false" : "true";
      }
      case ftUnionKey:
      case ftEnumKey: {
        if (field.optional) {
            return "None";
        }
        auto ev = field.value.type.enum_def->FindByValue(field.value.constant);
        assert(ev);
        return WrapInNameSpace(field.value.type.enum_def->defined_namespace,
                               GetEnumValue(*field.value.type.enum_def, *ev));
      }

      // All pointer-ish types have a default value of None, because they are
      // wrapped in Option.
      default: {
        return "None";
      }
    }
  }

  // Create the return type for fields in the *BuilderArgs structs that are
  // used to create Tables.
  //
  // Note: we could make all inputs to the BuilderArgs be an Option, as well
  // as all outputs. But, the UX of Flatbuffers is that the user doesn't get to
  // know if the value is default or not, because there are three ways to
  // return a default value:
  // 1) return a stored value that happens to be the default,
  // 2) return a hardcoded value because the relevant vtable field is not in
  //    the vtable, or
  // 3) return a hardcoded value because the vtable field value is set to zero.
  std::string TableBuilderArgsDefnType(const FieldDef &field,
                                       const std::string &lifetime) {
    const Type &type = field.value.type;

    switch (GetFullType(type)) {
      case ftInteger:
      case ftFloat:
      case ftBool: {
        const auto typname = GetTypeBasic(type);
        return field.optional ? "Option<" + typname + ">" : typname;
      }
      case ftStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "Option<&" + lifetime + " " + typname + ">";
      }
      case ftTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "Option<flatbuffers::WIPOffset<" + typname + "<" + lifetime +
               ">>>";
      }
      case ftString: {
        return "Option<flatbuffers::WIPOffset<&" + lifetime + " str>>";
      }
      case ftEnumKey:
      case ftUnionKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return field.optional ? "Option<" + typname + ">" : typname;
      }
      case ftUnionValue: {
        return "Option<flatbuffers::WIPOffset<flatbuffers::UnionWIPOffset>>";
      }

      case ftVectorOfInteger:
      case ftVectorOfBool:
      case ftVectorOfFloat: {
        const auto typname = GetTypeBasic(type.VectorType());
        return "Option<flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", " + typname + ">>>";
      }
      case ftVectorOfEnumKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return "Option<flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", " + typname + ">>>";
      }
      case ftVectorOfStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "Option<flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", " + typname + ">>>";
      }
      case ftVectorOfTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "Option<flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", flatbuffers::ForwardsUOffset<" + typname + "<" + lifetime +
               ">>>>>";
      }
      case ftVectorOfString: {
        return "Option<flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", flatbuffers::ForwardsUOffset<&" + lifetime + " str>>>>";
      }
      case ftVectorOfUnionValue: {
        const auto typname =
            WrapInNameSpace(*type.enum_def) + "UnionTableOffset";
        return "Option<flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", flatbuffers::ForwardsUOffset<"
               "flatbuffers::Table<" +
               lifetime + ">>>>";
      }
    }
    return "INVALID_CODE_GENERATION";  // for return analysis
  }

  std::string TableBuilderArgsAddFuncType(const FieldDef &field,
                                          const std::string &lifetime) {
    const Type &type = field.value.type;

    switch (GetFullType(field.value.type)) {
      case ftVectorOfStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime + ", " +
               typname + ">>";
      }
      case ftVectorOfTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", flatbuffers::ForwardsUOffset<" + typname + "<" + lifetime +
               ">>>>";
      }
      case ftVectorOfInteger:
      case ftVectorOfBool:
      case ftVectorOfFloat: {
        const auto typname = GetTypeBasic(type.VectorType());
        return "flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime + ", " +
               typname + ">>";
      }
      case ftVectorOfString: {
        return "flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", flatbuffers::ForwardsUOffset<&" + lifetime + " str>>>";
      }
      case ftVectorOfEnumKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return "flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime + ", " +
               typname + ">>";
      }
      case ftVectorOfUnionValue: {
        return "flatbuffers::WIPOffset<flatbuffers::Vector<" + lifetime +
               ", flatbuffers::ForwardsUOffset<flatbuffers::Table<" + lifetime +
               ">>>";
      }
      case ftEnumKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return typname;
      }
      case ftStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "&" + typname + "";
      }
      case ftTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "flatbuffers::WIPOffset<" + typname + "<" + lifetime + ">>";
      }
      case ftInteger:
      case ftBool:
      case ftFloat: {
        return GetTypeBasic(type);
      }
      case ftString: {
        return "flatbuffers::WIPOffset<&" + lifetime + " str>";
      }
      case ftUnionKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return typname;
      }
      case ftUnionValue: {
        return "flatbuffers::WIPOffset<flatbuffers::UnionWIPOffset>";
      }
    }

    return "INVALID_CODE_GENERATION";  // for return analysis
  }

  std::string TableBuilderArgsAddFuncBody(const FieldDef &field) {
    const Type &type = field.value.type;

    switch (GetFullType(field.value.type)) {
      case ftInteger:
      case ftBool:
      case ftFloat: {
        const auto typname = GetTypeBasic(field.value.type);
        return (field.optional ? "self.fbb_.push_slot_always::<"
                               : "self.fbb_.push_slot::<") +
               typname + ">";
      }
      case ftEnumKey:
      case ftUnionKey: {
        const auto underlying_typname = GetTypeBasic(type);
        return (field.optional ?
                   "self.fbb_.push_slot_always::<" :
                   "self.fbb_.push_slot::<") + underlying_typname + ">";
      }

      case ftStruct: {
        const std::string typname = WrapInNameSpace(*type.struct_def);
        return "self.fbb_.push_slot_always::<&" + typname + ">";
      }
      case ftTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return "self.fbb_.push_slot_always::<flatbuffers::WIPOffset<" +
               typname + ">>";
      }

      case ftUnionValue:
      case ftString:
      case ftVectorOfInteger:
      case ftVectorOfFloat:
      case ftVectorOfBool:
      case ftVectorOfEnumKey:
      case ftVectorOfStruct:
      case ftVectorOfTable:
      case ftVectorOfString:
      case ftVectorOfUnionValue: {
        return "self.fbb_.push_slot_always::<flatbuffers::WIPOffset<_>>";
      }
    }
    return "INVALID_CODE_GENERATION";  // for return analysis
  }

  std::string GenTableAccessorFuncReturnType(const FieldDef &field,
                                             const std::string &lifetime) {
    const Type &type = field.value.type;

    switch (GetFullType(field.value.type)) {
      case ftInteger:
      case ftFloat:
      case ftBool: {
        const auto typname = GetTypeBasic(type);
        return field.optional ? "Option<" + typname + ">" : typname;
      }
      case ftStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return WrapInOptionIfNotRequired("&" + lifetime + " " + typname,
                                         field.required);
      }
      case ftTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return WrapInOptionIfNotRequired(typname + "<" + lifetime + ">",
                                         field.required);
      }
      case ftEnumKey:
      case ftUnionKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return field.optional ? "Option<" + typname + ">" : typname;
      }

      case ftUnionValue: {
        return WrapInOptionIfNotRequired("flatbuffers::Table<" + lifetime + ">",
                                         field.required);
      }
      case ftString: {
        return WrapInOptionIfNotRequired("&" + lifetime + " str",
                                         field.required);
      }
      case ftVectorOfInteger:
      case ftVectorOfBool:
      case ftVectorOfFloat: {
        const auto typname = GetTypeBasic(type.VectorType());
        if (IsOneByte(type.VectorType().base_type)) {
          return WrapInOptionIfNotRequired(
              "&" + lifetime + " [" + typname + "]", field.required);
        }
        return WrapInOptionIfNotRequired(
            "flatbuffers::Vector<" + lifetime + ", " + typname + ">",
            field.required);
      }
      case ftVectorOfEnumKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return WrapInOptionIfNotRequired(
            "flatbuffers::Vector<" + lifetime + ", " + typname + ">",
            field.required);
      }
      case ftVectorOfStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return WrapInOptionIfNotRequired("&" + lifetime + " [" + typname + "]",
                                         field.required);
      }
      case ftVectorOfTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return WrapInOptionIfNotRequired("flatbuffers::Vector<" + lifetime +
                                             ", flatbuffers::ForwardsUOffset<" +
                                             typname + "<" + lifetime + ">>>",
                                         field.required);
      }
      case ftVectorOfString: {
        return WrapInOptionIfNotRequired(
            "flatbuffers::Vector<" + lifetime +
                ", flatbuffers::ForwardsUOffset<&" + lifetime + " str>>",
            field.required);
      }
      case ftVectorOfUnionValue: {
        FLATBUFFERS_ASSERT(false && "vectors of unions are not yet supported");
        // TODO(rw): when we do support these, we should consider using the
        //           Into trait to convert tables to typesafe union values.
        return "INVALID_CODE_GENERATION";  // for return analysis
      }
    }
    return "INVALID_CODE_GENERATION";  // for return analysis
  }

  std::string GenTableAccessorFuncBody(const FieldDef &field,
                                       const std::string &lifetime,
                                       const std::string &offset_prefix) {
    const std::string offset_name =
        offset_prefix + "::" + GetFieldOffsetName(field);
    const Type &type = field.value.type;

    switch (GetFullType(field.value.type)) {
      case ftInteger:
      case ftFloat:
      case ftBool: {
        const auto typname = GetTypeBasic(type);
        if (field.optional) {
          return "self._tab.get::<" + typname + ">(" + offset_name + ", None)";
        } else {
          const auto default_value = GetDefaultScalarValue(field);
          return "self._tab.get::<" + typname + ">(" + offset_name + ", Some(" +
                 default_value + ")).unwrap()";
        }
      }
      case ftStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return AddUnwrapIfRequired(
            "self._tab.get::<" + typname + ">(" + offset_name + ", None)",
            field.required);
      }
      case ftTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<" + typname + "<" +
                lifetime + ">>>(" + offset_name + ", None)",
            field.required);
      }
      case ftUnionValue: {
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<"
            "flatbuffers::Table<" +
                lifetime + ">>>(" + offset_name + ", None)",
            field.required);
      }
      case ftUnionKey:
      case ftEnumKey: {
        const std::string typname = WrapInNameSpace(*type.enum_def);
        const std::string default_value = GetDefaultScalarValue(field);
        if (field.optional) {
          return "self._tab.get::<" + typname + ">(" + offset_name + ", None)";
        } else {
          return "self._tab.get::<" + typname + ">(" + offset_name + ", Some(" +
                 default_value + ")).unwrap()";
        }
      }
      case ftString: {
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<&str>>(" +
                offset_name + ", None)",
            field.required);
      }

      case ftVectorOfInteger:
      case ftVectorOfBool:
      case ftVectorOfFloat: {
        const auto typname = GetTypeBasic(type.VectorType());
        std::string s =
            "self._tab.get::<flatbuffers::ForwardsUOffset<"
            "flatbuffers::Vector<" +
            lifetime + ", " + typname + ">>>(" + offset_name + ", None)";
        // single-byte values are safe to slice
        if (IsOneByte(type.VectorType().base_type)) {
          s += ".map(|v| v.safe_slice())";
        }
        return AddUnwrapIfRequired(s, field.required);
      }
      case ftVectorOfEnumKey: {
        const auto typname = WrapInNameSpace(*type.enum_def);
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<"
            "flatbuffers::Vector<" +
                lifetime + ", " + typname + ">>>(" + offset_name + ", None)",
            field.required);
      }
      case ftVectorOfStruct: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<"
            "flatbuffers::Vector<" +
                typname + ">>>(" + offset_name +
                ", None).map(|v| v.safe_slice() )",
            field.required);
      }
      case ftVectorOfTable: {
        const auto typname = WrapInNameSpace(*type.struct_def);
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<"
            "flatbuffers::Vector<flatbuffers::ForwardsUOffset<" +
                typname + "<" + lifetime + ">>>>>(" + offset_name + ", None)",
            field.required);
      }
      case ftVectorOfString: {
        return AddUnwrapIfRequired(
            "self._tab.get::<flatbuffers::ForwardsUOffset<"
            "flatbuffers::Vector<flatbuffers::ForwardsUOffset<&" +
                lifetime + " str>>>>(" + offset_name + ", None)",
            field.required);
      }
      case ftVectorOfUnionValue: {
        FLATBUFFERS_ASSERT(false && "vectors of unions are not yet supported");
        return "INVALID_CODE_GENERATION";  // for return analysis
      }
    }
    return "INVALID_CODE_GENERATION";  // for return analysis
  }

  bool TableFieldReturnsOption(const FieldDef &field) {
    if (field.optional) return true;
    switch (GetFullType(field.value.type)) {
      case ftInteger:
      case ftFloat:
      case ftBool:
      case ftEnumKey:
      case ftUnionKey: return false;
      default: return true;
    }
  }

  // Generates a fully-qualified name getter for use with --gen-name-strings
  void GenFullyQualifiedNameGetter(const StructDef &struct_def,
                                   const std::string &name) {
    code_ += "    pub const fn get_fully_qualified_name() -> &'static str {";
    code_ += "        \"" +
             struct_def.defined_namespace->GetFullyQualifiedName(name) + "\"";
    code_ += "    }";
    code_ += "";
  }

  void ForAllUnionVariantsBesidesNone(
    const EnumDef &def,
    std::function<void(const EnumVal &ev)> cb
  ) {
    FLATBUFFERS_ASSERT(def.is_union);

    for (auto it = def.Vals().begin(); it != def.Vals().end(); ++it) {
      const EnumVal & ev = **it;
      // TODO(cneo): Can variants be deprecated, should we skip them?
      if (ev.union_type.base_type == BASE_TYPE_NONE) { continue; }
      code_.SetValue(
          "U_ELEMENT_ENUM_TYPE",
          WrapInNameSpace(def.defined_namespace, GetEnumValue(def, ev)));
      code_.SetValue("U_ELEMENT_TABLE_TYPE",
          WrapInNameSpace(ev.union_type.struct_def->defined_namespace,
                          ev.union_type.struct_def->name));
      code_.SetValue("U_ELEMENT_NAME", MakeSnakeCase(Name(ev)));
      cb(ev);
    }
  }

  void ForAllTableFields(
    const StructDef &struct_def,
    std::function<void(const FieldDef&)> cb, bool reversed=false) {
    // TODO(cneo): Remove `reversed` overload. It's only here to minimize the
    // diff when refactoring to the `ForAllX` helper functions.
    auto go = [&](const FieldDef& field) {
      if (field.deprecated) return;
      code_.SetValue("OFFSET_NAME", GetFieldOffsetName(field));
      code_.SetValue("OFFSET_VALUE", NumToString(field.value.offset));
      code_.SetValue("FIELD_NAME", Name(field));
      code_.SetValue("DEFAULT_VALUE", GetDefaultScalarValue(field));
      cb(field);
    };
    const auto &fields = struct_def.fields.vec;
    if (reversed) {
      for (auto it = fields.rbegin(); it != fields.rend(); ++it) go(**it);
    } else {
      for (auto it = fields.begin(); it != fields.end(); ++it) go(**it);
    }
  }
  // Generate an accessor struct, builder struct, and create function for a
  // table.
  void GenTable(const StructDef &struct_def) {
    code_.SetValue("STRUCT_NAME", Name(struct_def));
    code_.SetValue("OFFSET_TYPELABEL", Name(struct_def) + "Offset");
    code_.SetValue("STRUCT_NAME_SNAKECASE", MakeSnakeCase(Name(struct_def)));

    // Generate an offset type, the base type, the Follow impl, and the
    // init_from_table impl.
    code_ += "pub enum {{OFFSET_TYPELABEL}} {}";
    code_ += "#[derive(Copy, Clone, PartialEq)]";
    code_ += "";

    GenComment(struct_def.doc_comment);

    code_ += "pub struct {{STRUCT_NAME}}<'a> {";
    code_ += "  pub _tab: flatbuffers::Table<'a>,";
    code_ += "}";
    code_ += "";
    code_ += "impl<'a> flatbuffers::Follow<'a> for {{STRUCT_NAME}}<'a> {";
    code_ += "    type Inner = {{STRUCT_NAME}}<'a>;";
    code_ += "    #[inline]";
    code_ += "    fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {";
    code_ += "        Self { _tab: flatbuffers::Table { buf, loc } }";
    code_ += "    }";
    code_ += "}";
    code_ += "";
    code_ += "impl<'a> {{STRUCT_NAME}}<'a> {";

    if (parser_.opts.generate_name_strings) {
      GenFullyQualifiedNameGetter(struct_def, struct_def.name);
    }

    code_ += "    #[inline]";
    code_ +=
        "    pub fn init_from_table(table: flatbuffers::Table<'a>) -> "
        "Self {";
    code_ += "        {{STRUCT_NAME}} {";
    code_ += "            _tab: table,";
    code_ += "        }";
    code_ += "    }";

    // Generate a convenient create* function that uses the above builder
    // to create a table in one function call.
    code_.SetValue("MAYBE_US", struct_def.fields.vec.size() == 0 ? "_" : "");
    code_.SetValue("MAYBE_LT",
                   TableBuilderArgsNeedsLifetime(struct_def) ? "<'args>" : "");
    code_ += "    #[allow(unused_mut)]";
    code_ += "    pub fn create<'bldr: 'args, 'args: 'mut_bldr, 'mut_bldr>(";
    code_ +=
        "        _fbb: "
        "&'mut_bldr mut flatbuffers::FlatBufferBuilder<'bldr>,";
    code_ +=
        "        {{MAYBE_US}}args: &'args {{STRUCT_NAME}}Args{{MAYBE_LT}})"
        " -> flatbuffers::WIPOffset<{{STRUCT_NAME}}<'bldr>> {";

    code_ += "      let mut builder = {{STRUCT_NAME}}Builder::new(_fbb);";
    for (size_t size = struct_def.sortbysize ? sizeof(largest_scalar_t) : 1;
         size; size /= 2) {
      ForAllTableFields(struct_def, [&](const FieldDef &field) {
        if (struct_def.sortbysize && size != SizeOf(field.value.type.base_type))
          return;
        if (TableFieldReturnsOption(field)) {
          code_ +=
              "      if let Some(x) = args.{{FIELD_NAME}} "
              "{ builder.add_{{FIELD_NAME}}(x); }";
        } else {
          code_ += "      builder.add_{{FIELD_NAME}}(args.{{FIELD_NAME}});";
        }
      }, /*reverse=*/true);
    }
    code_ += "      builder.finish()";
    code_ += "    }";
    code_ += "";

    // Generate field id constants.
    ForAllTableFields(struct_def, [&](const FieldDef &unused){
      (void) unused;
      code_ += "    pub const {{OFFSET_NAME}}: flatbuffers::VOffsetT = "
               "{{OFFSET_VALUE}};";
    });
    if (struct_def.fields.vec.size() > 0) code_ += "";

    // Generate the accessors. Each has one of two forms:
    //
    // If a value can be None:
    //   pub fn name(&'a self) -> Option<user_facing_type> {
    //     self._tab.get::<internal_type>(offset, defaultval)
    //   }
    //
    // If a value is always Some:
    //   pub fn name(&'a self) -> user_facing_type {
    //     self._tab.get::<internal_type>(offset, defaultval).unwrap()
    //   }
    const auto offset_prefix = Name(struct_def);
    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      code_.SetValue("RETURN_TYPE",
                     GenTableAccessorFuncReturnType(field, "'a"));
      code_.SetValue("FUNC_BODY",
                     GenTableAccessorFuncBody(field, "'a", offset_prefix));

      this->GenComment(field.doc_comment, "  ");
      code_ += "  #[inline]";
      code_ += "  pub fn {{FIELD_NAME}}(&self) -> {{RETURN_TYPE}} {";
      code_ += "    {{FUNC_BODY}}";
      code_ += "  }";

      // Generate a comparison function for this field if it is a key.
      if (field.key) { GenKeyFieldMethods(field); }

      // Generate a nested flatbuffer field, if applicable.
      auto nested = field.attributes.Lookup("nested_flatbuffer");
      if (nested) {
        std::string qualified_name = nested->constant;
        auto nested_root = parser_.LookupStruct(nested->constant);
        if (nested_root == nullptr) {
          qualified_name = parser_.current_namespace_->GetFullyQualifiedName(
              nested->constant);
          nested_root = parser_.LookupStruct(qualified_name);
        }
        FLATBUFFERS_ASSERT(nested_root);  // Guaranteed to exist by parser.

        code_.SetValue("NESTED", WrapInNameSpace(*nested_root));
        code_ +=
            "  pub fn {{FIELD_NAME}}_nested_flatbuffer(&'a self) -> "
            "Option<{{NESTED}}<'a>> {";
        code_ += "    self.{{FIELD_NAME}}().map(|data| {";
        code_ += "      use flatbuffers::Follow;";
        code_ += "      <flatbuffers::ForwardsUOffset<{{NESTED}}<'a>>>"
            "::follow(data, 0)";
        code_ += "    })";
        code_ += "  }";
      }
    });

    // Explicit specializations for union accessors
    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      if (field.value.type.base_type != BASE_TYPE_UNION) return;
      code_.SetValue("FIELD_TYPE_FIELD_NAME", field.name);
      ForAllUnionVariantsBesidesNone(
        *field.value.type.enum_def, [&](const EnumVal &unused){
        (void) unused;
        code_ += "  #[inline]";
        code_ += "  #[allow(non_snake_case)]";
        code_ +=
            "  pub fn {{FIELD_NAME}}_as_{{U_ELEMENT_NAME}}(&self) -> "
            "Option<{{U_ELEMENT_TABLE_TYPE}}<'a>> {";
        // If the user defined schemas name a field that clashes with a
        // language reserved word, flatc will try to escape the field name by
        // appending an underscore. This works well for most cases, except
        // one. When generating union accessors (and referring to them
        // internally within the code generated here), an extra underscore
        // will be appended to the name, causing build failures.
        //
        // This only happens when unions have members that overlap with
        // language reserved words.
        //
        // To avoid this problem the type field name is used unescaped here:
        code_ +=
            "    if self.{{FIELD_TYPE_FIELD_NAME}}_type() == "
            "{{U_ELEMENT_ENUM_TYPE}} {";

        // The following logic is not tested in the integration test,
        // as of April 10, 2020
        if (field.required) {
          code_ += "      let u = self.{{FIELD_NAME}}();";
          code_ += "      Some({{U_ELEMENT_TABLE_TYPE}}::init_from_table(u))";
        } else {
          code_ +=
              "      self.{{FIELD_NAME}}().map("
              "{{U_ELEMENT_TABLE_TYPE}}::init_from_table)";
        }
        code_ += "    } else {";
        code_ += "      None";
        code_ += "    }";
        code_ += "  }";
        code_ += "";

      });
    });
    code_ += "}";  // End of table impl.
    code_ += "";

    // Generate an args struct:
    code_.SetValue("MAYBE_LT",
                   TableBuilderArgsNeedsLifetime(struct_def) ? "<'a>" : "");
    code_ += "pub struct {{STRUCT_NAME}}Args{{MAYBE_LT}} {";
    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      code_.SetValue("PARAM_TYPE", TableBuilderArgsDefnType(field, "'a"));
      code_ += "    pub {{FIELD_NAME}}: {{PARAM_TYPE}},";
    });
    code_ += "}";

    // Generate an impl of Default for the *Args type:
    code_ += "impl<'a> Default for {{STRUCT_NAME}}Args{{MAYBE_LT}} {";
    code_ += "    #[inline]";
    code_ += "    fn default() -> Self {";
    code_ += "        {{STRUCT_NAME}}Args {";
    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      code_ += "            {{FIELD_NAME}}: {{DEFAULT_VALUE}},\\";
      code_ += field.required ? " // required field" : "";
    });
    code_ += "        }";
    code_ += "    }";
    code_ += "}";

    // Generate a builder struct:
    code_ += "pub struct {{STRUCT_NAME}}Builder<'a: 'b, 'b> {";
    code_ += "  fbb_: &'b mut flatbuffers::FlatBufferBuilder<'a>,";
    code_ +=
        "  start_: flatbuffers::WIPOffset<"
        "flatbuffers::TableUnfinishedWIPOffset>,";
    code_ += "}";

    // Generate builder functions:
    code_ += "impl<'a: 'b, 'b> {{STRUCT_NAME}}Builder<'a, 'b> {";
    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      const bool is_scalar = IsScalar(field.value.type.base_type);
      std::string offset = GetFieldOffsetName(field);
      // Generate functions to add data, which take one of two forms.
      //
      // If a value has a default:
      //   fn add_x(x_: type) {
      //     fbb_.push_slot::<type>(offset, x_, Some(default));
      //   }
      //
      // If a value does not have a default:
      //   fn add_x(x_: type) {
      //     fbb_.push_slot_always::<type>(offset, x_);
      //   }
      code_.SetValue("FIELD_OFFSET", Name(struct_def) + "::" + offset);
      code_.SetValue("FIELD_TYPE", TableBuilderArgsAddFuncType(field, "'b "));
      code_.SetValue("FUNC_BODY", TableBuilderArgsAddFuncBody(field));
      code_ += "  #[inline]";
      code_ += "  pub fn add_{{FIELD_NAME}}(&mut self, {{FIELD_NAME}}: "
               "{{FIELD_TYPE}}) {";
      if (is_scalar && !field.optional) {
        code_ +=
            "    {{FUNC_BODY}}({{FIELD_OFFSET}}, {{FIELD_NAME}}, "
            "{{DEFAULT_VALUE}});";
      } else {
        code_ += "    {{FUNC_BODY}}({{FIELD_OFFSET}}, {{FIELD_NAME}});";
      }
      code_ += "  }";
    });

    // Struct initializer (all fields required);
    code_ += "  #[inline]";
    code_ +=
        "  pub fn new(_fbb: &'b mut flatbuffers::FlatBufferBuilder<'a>) -> "
        "{{STRUCT_NAME}}Builder<'a, 'b> {";
    code_.SetValue("NUM_FIELDS", NumToString(struct_def.fields.vec.size()));
    code_ += "    let start = _fbb.start_table();";
    code_ += "    {{STRUCT_NAME}}Builder {";
    code_ += "      fbb_: _fbb,";
    code_ += "      start_: start,";
    code_ += "    }";
    code_ += "  }";

    // finish() function.
    code_ += "  #[inline]";
    code_ +=
        "  pub fn finish(self) -> "
        "flatbuffers::WIPOffset<{{STRUCT_NAME}}<'a>> {";
    code_ += "    let o = self.fbb_.end_table(self.start_);";

    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      if (!field.required) return;
      code_ +=
          "    self.fbb_.required(o, {{STRUCT_NAME}}::{{OFFSET_NAME}},"
          "\"{{FIELD_NAME}}\");";
    });
    code_ += "    flatbuffers::WIPOffset::new(o.value())";
    code_ += "  }";
    code_ += "}";
    code_ += "";

    code_ += "impl std::fmt::Debug for {{STRUCT_NAME}}<'_> {";
    code_ += "  fn fmt(&self, f: &mut std::fmt::Formatter<'_>"
             ") -> std::fmt::Result {";
    code_ += "    let mut ds = f.debug_struct(\"{{STRUCT_NAME}}\");";
    ForAllTableFields(struct_def, [&](const FieldDef &field) {
      if (GetFullType(field.value.type) == ftUnionValue) {
        // Generate a match statement to handle unions properly.
        code_.SetValue("KEY_TYPE", GenTableAccessorFuncReturnType(field, ""));
        code_.SetValue("FIELD_TYPE_FIELD_NAME", field.name);
        code_.SetValue("UNION_ERR", "&\"InvalidFlatbuffer: Union discriminant"
                                    " does not match value.\"");

        code_ += "      match self.{{FIELD_NAME}}_type() {";
        ForAllUnionVariantsBesidesNone(*field.value.type.enum_def,
                                       [&](const EnumVal &unused){
          (void) unused;
          code_ += "        {{U_ELEMENT_ENUM_TYPE}} => {";
          code_ += "          if let Some(x) = self.{{FIELD_TYPE_FIELD_NAME}}_as_"
                   "{{U_ELEMENT_NAME}}() {";
          code_ += "            ds.field(\"{{FIELD_NAME}}\", &x)";
          code_ += "          } else {";
          code_ += "            ds.field(\"{{FIELD_NAME}}\", {{UNION_ERR}})";
          code_ += "          }";
          code_ += "        },";
        });
        code_ += "        _ => { ";
        code_ += "          let x: Option<()> = None;";
        code_ += "          ds.field(\"{{FIELD_NAME}}\", &x)";
        code_ += "        },";
        code_ += "      };";
      } else {
        // Most fields.
        code_ += "      ds.field(\"{{FIELD_NAME}}\", &self.{{FIELD_NAME}}());";
      }
    });
    code_ += "      ds.finish()";
    code_ += "  }";
    code_ += "}";
  }

  // Generate functions to compare tables and structs by key. This function
  // must only be called if the field key is defined.
  void GenKeyFieldMethods(const FieldDef &field) {
    FLATBUFFERS_ASSERT(field.key);

    code_.SetValue("KEY_TYPE", GenTableAccessorFuncReturnType(field, ""));

    code_ += "  #[inline]";
    code_ +=
        "  pub fn key_compare_less_than(&self, o: &{{STRUCT_NAME}}) -> "
        " bool {";
    code_ += "    self.{{FIELD_NAME}}() < o.{{FIELD_NAME}}()";
    code_ += "  }";
    code_ += "";
    code_ += "  #[inline]";
    code_ +=
        "  pub fn key_compare_with_value(&self, val: {{KEY_TYPE}}) -> "
        " ::std::cmp::Ordering {";
    code_ += "    let key = self.{{FIELD_NAME}}();";
    code_ += "    key.cmp(&val)";
    code_ += "  }";
  }

  // Generate functions for accessing the root table object. This function
  // must only be called if the root table is defined.
  void GenRootTableFuncs(const StructDef &struct_def) {
    FLATBUFFERS_ASSERT(parser_.root_struct_def_ && "root table not defined");
    auto name = Name(struct_def);

    code_.SetValue("STRUCT_NAME", name);
    code_.SetValue("STRUCT_NAME_SNAKECASE", MakeSnakeCase(name));
    code_.SetValue("STRUCT_NAME_CAPS", MakeUpper(MakeSnakeCase(name)));

    // The root datatype accessors:
    code_ += "#[inline]";
    code_ +=
        "pub fn get_root_as_{{STRUCT_NAME_SNAKECASE}}<'a>(buf: &'a [u8])"
        " -> {{STRUCT_NAME}}<'a> {";
    code_ += "  flatbuffers::get_root::<{{STRUCT_NAME}}<'a>>(buf)";
    code_ += "}";
    code_ += "";

    code_ += "#[inline]";
    code_ +=
        "pub fn get_size_prefixed_root_as_{{STRUCT_NAME_SNAKECASE}}"
        "<'a>(buf: &'a [u8]) -> {{STRUCT_NAME}}<'a> {";
    code_ +=
        "  flatbuffers::get_size_prefixed_root::<{{STRUCT_NAME}}<'a>>"
        "(buf)";
    code_ += "}";
    code_ += "";

    if (parser_.file_identifier_.length()) {
      // Declare the identifier
      // (no lifetime needed as constants have static lifetimes by default)
      code_ += "pub const {{STRUCT_NAME_CAPS}}_IDENTIFIER: &str\\";
      code_ += " = \"" + parser_.file_identifier_ + "\";";
      code_ += "";

      // Check if a buffer has the identifier.
      code_ += "#[inline]";
      code_ += "pub fn {{STRUCT_NAME_SNAKECASE}}_buffer_has_identifier\\";
      code_ += "(buf: &[u8]) -> bool {";
      code_ += "  flatbuffers::buffer_has_identifier(buf, \\";
      code_ += "{{STRUCT_NAME_CAPS}}_IDENTIFIER, false)";
      code_ += "}";
      code_ += "";
      code_ += "#[inline]";
      code_ += "pub fn {{STRUCT_NAME_SNAKECASE}}_size_prefixed\\";
      code_ += "_buffer_has_identifier(buf: &[u8]) -> bool {";
      code_ += "  flatbuffers::buffer_has_identifier(buf, \\";
      code_ += "{{STRUCT_NAME_CAPS}}_IDENTIFIER, true)";
      code_ += "}";
      code_ += "";
    }

    if (parser_.file_extension_.length()) {
      // Return the extension
      code_ += "pub const {{STRUCT_NAME_CAPS}}_EXTENSION: &str = \\";
      code_ += "\"" + parser_.file_extension_ + "\";";
      code_ += "";
    }

    // Finish a buffer with a given root object:
    code_.SetValue("OFFSET_TYPELABEL", Name(struct_def) + "Offset");
    code_ += "#[inline]";
    code_ += "pub fn finish_{{STRUCT_NAME_SNAKECASE}}_buffer<'a, 'b>(";
    code_ += "    fbb: &'b mut flatbuffers::FlatBufferBuilder<'a>,";
    code_ += "    root: flatbuffers::WIPOffset<{{STRUCT_NAME}}<'a>>) {";
    if (parser_.file_identifier_.length()) {
      code_ += "  fbb.finish(root, Some({{STRUCT_NAME_CAPS}}_IDENTIFIER));";
    } else {
      code_ += "  fbb.finish(root, None);";
    }
    code_ += "}";
    code_ += "";
    code_ += "#[inline]";
    code_ +=
        "pub fn finish_size_prefixed_{{STRUCT_NAME_SNAKECASE}}_buffer"
        "<'a, 'b>("
        "fbb: &'b mut flatbuffers::FlatBufferBuilder<'a>, "
        "root: flatbuffers::WIPOffset<{{STRUCT_NAME}}<'a>>) {";
    if (parser_.file_identifier_.length()) {
      code_ +=
          "  fbb.finish_size_prefixed(root, "
          "Some({{STRUCT_NAME_CAPS}}_IDENTIFIER));";
    } else {
      code_ += "  fbb.finish_size_prefixed(root, None);";
    }
    code_ += "}";
  }

  static void GenPadding(
      const FieldDef &field, std::string *code_ptr, int *id,
      const std::function<void(int bits, std::string *code_ptr, int *id)> &f) {
    if (field.padding) {
      for (int i = 0; i < 4; i++) {
        if (static_cast<int>(field.padding) & (1 << i)) {
          f((1 << i) * 8, code_ptr, id);
        }
      }
      assert(!(field.padding & ~0xF));
    }
  }

  static void PaddingDefinition(int bits, std::string *code_ptr, int *id) {
    *code_ptr +=
        "  padding" + NumToString((*id)++) + "__: u" + NumToString(bits) + ",";
  }

  static void PaddingInitializer(int bits, std::string *code_ptr, int *id) {
    (void)bits;
    *code_ptr += "padding" + NumToString((*id)++) + "__: 0,";
  }

  void ForAllStructFields(
    const StructDef &struct_def,
    std::function<void(const FieldDef &field)> cb
  ) {
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      const auto &field = **it;
      code_.SetValue("FIELD_TYPE", GetTypeGet(field.value.type));
      code_.SetValue("FIELD_NAME", Name(field));
      cb(field);
    }
  }
  // Generate an accessor struct with constructor for a flatbuffers struct.
  void GenStruct(const StructDef &struct_def) {
    // Generates manual padding and alignment.
    // Variables are private because they contain little endian data on all
    // platforms.
    GenComment(struct_def.doc_comment);
    code_.SetValue("ALIGN", NumToString(struct_def.minalign));
    code_.SetValue("STRUCT_NAME", Name(struct_def));

    code_ += "// struct {{STRUCT_NAME}}, aligned to {{ALIGN}}";
    code_ += "#[repr(C, align({{ALIGN}}))]";

    // PartialEq is useful to derive because we can correctly compare structs
    // for equality by just comparing their underlying byte data. This doesn't
    // hold for PartialOrd/Ord.
    code_ += "#[derive(Clone, Copy, PartialEq)]";
    code_ += "pub struct {{STRUCT_NAME}} {";

    int padding_id = 0;
    ForAllStructFields(struct_def, [&](const FieldDef &field) {
      code_ += "  {{FIELD_NAME}}_: {{FIELD_TYPE}},";
      if (field.padding) {
        std::string padding;
        GenPadding(field, &padding, &padding_id, PaddingDefinition);
        code_ += padding;
      }
    });
    code_ += "} // pub struct {{STRUCT_NAME}}";

    // Debug for structs.
    code_ += "impl std::fmt::Debug for {{STRUCT_NAME}} {";
    code_ += "  fn fmt(&self, f: &mut std::fmt::Formatter"
             ") -> std::fmt::Result {";
    code_ += "    f.debug_struct(\"{{STRUCT_NAME}}\")";
    ForAllStructFields(struct_def, [&](const FieldDef &unused) {
      (void) unused;
      code_ += "      .field(\"{{FIELD_NAME}}\", &self.{{FIELD_NAME}}())";
    });
    code_ += "      .finish()";
    code_ += "  }";
    code_ += "}";
    code_ += "";


    // Generate impls for SafeSliceAccess (because all structs are endian-safe),
    // Follow for the value type, Follow for the reference type, Push for the
    // value type, and Push for the reference type.
    code_ += "impl flatbuffers::SafeSliceAccess for {{STRUCT_NAME}} {}";
    code_ += "impl<'a> flatbuffers::Follow<'a> for {{STRUCT_NAME}} {";
    code_ += "  type Inner = &'a {{STRUCT_NAME}};";
    code_ += "  #[inline]";
    code_ += "  fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {";
    code_ += "    <&'a {{STRUCT_NAME}}>::follow(buf, loc)";
    code_ += "  }";
    code_ += "}";
    code_ += "impl<'a> flatbuffers::Follow<'a> for &'a {{STRUCT_NAME}} {";
    code_ += "  type Inner = &'a {{STRUCT_NAME}};";
    code_ += "  #[inline]";
    code_ += "  fn follow(buf: &'a [u8], loc: usize) -> Self::Inner {";
    code_ += "    flatbuffers::follow_cast_ref::<{{STRUCT_NAME}}>(buf, loc)";
    code_ += "  }";
    code_ += "}";
    code_ += "impl<'b> flatbuffers::Push for {{STRUCT_NAME}} {";
    code_ += "    type Output = {{STRUCT_NAME}};";
    code_ += "    #[inline]";
    code_ += "    fn push(&self, dst: &mut [u8], _rest: &[u8]) {";
    code_ += "        let src = unsafe {";
    code_ +=
        "            ::std::slice::from_raw_parts("
        "self as *const {{STRUCT_NAME}} as *const u8, Self::size())";
    code_ += "        };";
    code_ += "        dst.copy_from_slice(src);";
    code_ += "    }";
    code_ += "}";
    code_ += "impl<'b> flatbuffers::Push for &'b {{STRUCT_NAME}} {";
    code_ += "    type Output = {{STRUCT_NAME}};";
    code_ += "";
    code_ += "    #[inline]";
    code_ += "    fn push(&self, dst: &mut [u8], _rest: &[u8]) {";
    code_ += "        let src = unsafe {";
    code_ +=
        "            ::std::slice::from_raw_parts("
        "*self as *const {{STRUCT_NAME}} as *const u8, Self::size())";
    code_ += "        };";
    code_ += "        dst.copy_from_slice(src);";
    code_ += "    }";
    code_ += "}";
    code_ += "";
    code_ += "";

    // Generate a constructor that takes all fields as arguments.
    code_ += "impl {{STRUCT_NAME}} {";
    // TODO(cneo): Stop generating args on one line. Make it simpler.
    bool first_arg = true;
    code_ += "  pub fn new(\\";
    ForAllStructFields(struct_def, [&](const FieldDef &field) {
      if (first_arg) first_arg = false; else code_ += ", \\";
      code_.SetValue("REF", IsStruct(field.value.type) ? "&" : "");
      code_ += "_{{FIELD_NAME}}: {{REF}}{{FIELD_TYPE}}\\";
    });
    code_ += ") -> Self {";
    code_ += "    {{STRUCT_NAME}} {";

    ForAllStructFields(struct_def, [&](const FieldDef &field) {
      const bool is_struct = IsStruct(field.value.type);
      code_.SetValue("DEREF", is_struct ? "*" : "");
      code_.SetValue("TO_LE", is_struct ? "" : ".to_little_endian()");
      code_ += "      {{FIELD_NAME}}_: {{DEREF}}_{{FIELD_NAME}}{{TO_LE}},";
    });
    code_ += "";

    // TODO(cneo): Does this padding even work? Why after all the fields?
    padding_id = 0;
    ForAllStructFields(struct_def, [&](const FieldDef &field) {
      if (field.padding) {
        std::string padding;
        GenPadding(field, &padding, &padding_id, PaddingInitializer);
        code_ += "      " + padding;
      }
    });
    code_ += "    }";
    code_ += "  }";

    if (parser_.opts.generate_name_strings) {
      GenFullyQualifiedNameGetter(struct_def, struct_def.name);
    }

    // Generate accessor methods for the struct.
    ForAllStructFields(struct_def, [&](const FieldDef &field) {
      const bool is_struct = IsStruct(field.value.type);
      code_.SetValue("REF", is_struct ? "&" : "");
      code_.SetValue("FROM_LE", is_struct ? "" : ".from_little_endian()");

      this->GenComment(field.doc_comment, "  ");
      code_ += "  pub fn {{FIELD_NAME}}(&self) -> {{REF}}{{FIELD_TYPE}} {";
      code_ += "    {{REF}}self.{{FIELD_NAME}}_{{FROM_LE}}";
      code_ += "  }";

      // Generate a comparison function for this field if it is a key.
      if (field.key) { GenKeyFieldMethods(field); }
    });
    code_ += "}";
    code_ += "";
  }

  void GenNamespaceImports(const int white_spaces) {
    if (white_spaces == 0) {
      code_ += "#![allow(unused_imports, dead_code)]";
    }
    std::string indent = std::string(white_spaces, ' ');
    code_ += "";
    if (!parser_.opts.generate_all) {
      for (auto it = parser_.included_files_.begin();
           it != parser_.included_files_.end(); ++it) {
        if (it->second.empty()) continue;
        auto noext = flatbuffers::StripExtension(it->second);
        auto basename = flatbuffers::StripPath(noext);

        code_ += indent + "use crate::" + basename + "_generated::*;";
      }
    }

    code_ += indent + "use std::mem;";
    code_ += indent + "use std::cmp::Ordering;";
    code_ += "";
    code_ += indent + "extern crate flatbuffers;";
    code_ += indent + "use self::flatbuffers::EndianScalar;";
  }

  // Set up the correct namespace. This opens a namespace if the current
  // namespace is different from the target namespace. This function
  // closes and opens the namespaces only as necessary.
  //
  // The file must start and end with an empty (or null) namespace so that
  // namespaces are properly opened and closed.
  void SetNameSpace(const Namespace *ns) {
    if (cur_name_space_ == ns) { return; }

    // Compute the size of the longest common namespace prefix.
    // If cur_name_space is A::B::C::D and ns is A::B::E::F::G,
    // the common prefix is A::B:: and we have old_size = 4, new_size = 5
    // and common_prefix_size = 2
    size_t old_size = cur_name_space_ ? cur_name_space_->components.size() : 0;
    size_t new_size = ns ? ns->components.size() : 0;

    size_t common_prefix_size = 0;
    while (common_prefix_size < old_size && common_prefix_size < new_size &&
           ns->components[common_prefix_size] ==
               cur_name_space_->components[common_prefix_size]) {
      common_prefix_size++;
    }

    // Close cur_name_space in reverse order to reach the common prefix.
    // In the previous example, D then C are closed.
    for (size_t j = old_size; j > common_prefix_size; --j) {
      code_ += "}  // pub mod " + cur_name_space_->components[j - 1];
    }
    if (old_size != common_prefix_size) { code_ += ""; }

    // open namespace parts to reach the ns namespace
    // in the previous example, E, then F, then G are opened
    for (auto j = common_prefix_size; j != new_size; ++j) {
      code_ += "#[allow(unused_imports, dead_code)]";
      code_ += "pub mod " + MakeSnakeCase(ns->components[j]) + " {";
      // Generate local namespace imports.
      GenNamespaceImports(2);
    }
    if (new_size != common_prefix_size) { code_ += ""; }

    cur_name_space_ = ns;
  }
};

}  // namespace rust

bool GenerateRust(const Parser &parser, const std::string &path,
                  const std::string &file_name) {
  rust::RustGenerator generator(parser, path, file_name);
  return generator.generate();
}

std::string RustMakeRule(const Parser &parser, const std::string &path,
                         const std::string &file_name) {
  std::string filebase =
      flatbuffers::StripPath(flatbuffers::StripExtension(file_name));
  rust::RustGenerator generator(parser, path, file_name);
  std::string make_rule =
      generator.GeneratedFileName(path, filebase, parser.opts) + ": ";

  auto included_files = parser.GetIncludedFilesRecursive(file_name);
  for (auto it = included_files.begin(); it != included_files.end(); ++it) {
    make_rule += " " + *it;
  }
  return make_rule;
}

}  // namespace flatbuffers

// TODO(rw): Generated code should import other generated files.
// TODO(rw): Generated code should refer to namespaces in included files in a
//           way that makes them referrable.
// TODO(rw): Generated code should indent according to nesting level.
// TODO(rw): Generated code should generate endian-safe Debug impls.
// TODO(rw): Generated code could use a Rust-only enum type to access unions,
//           instead of making the user use _type() to manually switch.
// TODO(maxburke): There should be test schemas added that use language
//           keywords as fields of structs, tables, unions, enums, to make sure
//           that internal code generated references escaped names correctly.
// TODO(maxburke): We should see if there is a more flexible way of resolving
//           module paths for use declarations. Right now if schemas refer to
//           other flatbuffer files, the include paths in emitted Rust bindings
//           are crate-relative which may undesirable.