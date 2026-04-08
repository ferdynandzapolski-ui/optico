import os
import lit.formats

config.name = "GOIR"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.ll', '.c']
config.test_source_root = os.path.dirname(__file__)

config.substitutions.append(('%opt', 'opt -load-pass-plugin=' + config.goir_passes_dir + '/GOIRPasses.so'))
