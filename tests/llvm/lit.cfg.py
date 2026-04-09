import os
import lit.formats

config.name = "GOIR-LLVM"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.ll']
config.test_source_root = os.path.dirname(__file__)

# config.goir_passes_dir is expected to be set by the site config
config.substitutions.append(('%opt', 'opt -load-pass-plugin=' + config.goir_passes_dir + '/GOIRPasses.so'))
