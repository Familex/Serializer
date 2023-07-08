#pragma once

namespace dynser::config::keywords
{

#define DYNSER_NEW_KEYWORD inline constexpr auto

//==== lua

DYNSER_NEW_KEYWORD INPUT_TABLE = "inp";    // 'in' is a lua keyword
DYNSER_NEW_KEYWORD OUTPUT_TABLE = "out";
DYNSER_NEW_KEYWORD CONTEXT = "ctx";
DYNSER_NEW_KEYWORD BRANCHED_RULE_IND_VARIABLE = "branch";
DYNSER_NEW_KEYWORD BRANCHED_RULE_IND_ERRVAL = -1;    // not exactly keyword

//==== lua

//==== yaml

DYNSER_NEW_KEYWORD VERSION = "version";
DYNSER_NEW_KEYWORD TAGS = "tags";

DYNSER_NEW_KEYWORD NAME = "name";
DYNSER_NEW_KEYWORD NESTED_CONTINUAL = "continual";
DYNSER_NEW_KEYWORD NESTED_RECURRENT = "recurrent";
DYNSER_NEW_KEYWORD NESTED_BRANCHED = "branched";
DYNSER_NEW_KEYWORD SERIALIZATION_SCRIPT = "serialization-script";
DYNSER_NEW_KEYWORD DESERIALIZATION_SCRIPT = "deserialization-script";

DYNSER_NEW_KEYWORD CONTINUAL_EXISTING = "existing";
DYNSER_NEW_KEYWORD CONTINUAL_LINEAR = "linear";
DYNSER_NEW_KEYWORD RECURRENT_EXISTING = "existing";
DYNSER_NEW_KEYWORD RECURRENT_LINEAR = "linear";
DYNSER_NEW_KEYWORD RECURRENT_INFIX = "infix";
DYNSER_NEW_KEYWORD BRANCHED_BRANCHING_SCRIPT = "branching-script";
DYNSER_NEW_KEYWORD BRANCHED_DEBRANCHING_SCRIPT = "debranching-script";
DYNSER_NEW_KEYWORD BRANCHED_RULES = "rules";
DYNSER_NEW_KEYWORD BRANCHED_EXISTING = "existing";
DYNSER_NEW_KEYWORD BRANCHED_LINEAR = "linear";

DYNSER_NEW_KEYWORD CONTINUAL_EXISTING_TAG = "tag";
DYNSER_NEW_KEYWORD CONTINUAL_EXISTING_PREFIX = "prefix";
DYNSER_NEW_KEYWORD CONTINUAL_LINEAR_PATTERN = "pattern";
DYNSER_NEW_KEYWORD CONTINUAL_LINEAR_DYN_GROUPS = "dyn-groups";
DYNSER_NEW_KEYWORD CONTINUAL_LINEAR_FIELDS = "fields";
DYNSER_NEW_KEYWORD RECURRENT_EXISTING_TAG = "tag";
DYNSER_NEW_KEYWORD RECURRENT_EXISTING_PREFIX = "prefix";
DYNSER_NEW_KEYWORD RECURRENT_EXISTING_WRAP = "wrap";
DYNSER_NEW_KEYWORD RECURRENT_EXISTING_DEFAULT_VALUE = "default";
DYNSER_NEW_KEYWORD RECURRENT_LINEAR_PATTERN = "pattern";
DYNSER_NEW_KEYWORD RECURRENT_LINEAR_DYN_GROUPS = "dyn-groups";
DYNSER_NEW_KEYWORD RECURRENT_LINEAR_FIELDS = "fields";
DYNSER_NEW_KEYWORD RECURRENT_LINEAR_WRAP = "wrap";
DYNSER_NEW_KEYWORD RECURRENT_LINEAR_DEFAULT_VALUE = "default";
DYNSER_NEW_KEYWORD RECURRENT_INFIX_PATTERN = "pattern";
DYNSER_NEW_KEYWORD BRANCHED_EXISTING_TAG = "tag";
DYNSER_NEW_KEYWORD BRANCHED_EXISTING_PREFIX = "prefix";
DYNSER_NEW_KEYWORD BRANCHED_LINEAR_PATTERN = "pattern";
DYNSER_NEW_KEYWORD BRANCHED_LINEAR_DYN_GROUPS = "dyn-groups";
DYNSER_NEW_KEYWORD BRANCHED_LINEAR_FIELDS = "fields";

//==== yaml

#undef NEW_KEYWORD

}    // namespace dynser::config::keywords