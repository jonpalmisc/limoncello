#!/usr/bin/env python3

from argparse import ArgumentParser
from dataclasses import dataclass
from enum import Enum
from fnmatch import fnmatch
import multiprocessing
import subprocess
from typing import Tuple

SOURCE_DIR = "test/Samples"
CONFIG_DIR = "test/Configs"
BUILD_DIR = "test/Samples/Output"


class SampleType(Enum):
    EXECUTABLE = 0
    LIBRARY = 1


@dataclass
class Context:
    clang_path: str
    plugin_path: str


@dataclass
class Sample:
    type: SampleType
    source: str
    config: str

    def name(self):
        return self.source.split(".", 1)[0]

    def output_name(self, opt_type: str, with_obfuscation: bool = True):
        tag = opt_type
        if with_obfuscation:
            tag += f",{self.config}"

        return f"{self.name()}.{tag}"

    def build(self, context: Context, opt_type: str, with_obfuscation: bool = True):
        output_path = f"{BUILD_DIR}/{self.output_name(opt_type, with_obfuscation)}"
        source_path = f"{SOURCE_DIR}/{self.source}"

        args = [context.clang_path]
        if self.type == SampleType.LIBRARY:
            args += ["-shared"]
        if with_obfuscation:
            args += [
                f"-fplugin={context.plugin_path}",
                f"-fpass-plugin={context.plugin_path}",
                "-mllvm",
                f"-limoncello-config={CONFIG_DIR}/{self.config}.yml",
            ]
        args += ["-" + opt_type, "-o", output_path, source_path]

        subprocess.run(args)


ALL_SAMPLES = [
    Sample(SampleType.EXECUTABLE, "ArithmeticBonanza.c", "ArithmeticMangler"),
    Sample(SampleType.EXECUTABLE, "ConstantPaloozaRedux.c", "ConstantMangler"),
    Sample(SampleType.EXECUTABLE, "DoubleSwitch.c", "Flattener"),
    Sample(SampleType.EXECUTABLE, "Hello.c", "Default"),
    Sample(SampleType.EXECUTABLE, "NumberClassifier.c", "Flattener"),
    Sample(SampleType.EXECUTABLE, "NumberClassifier.c", "FlattenerRandomIDs"),
    Sample(SampleType.EXECUTABLE, "SimpleBlocks.c", "Bloater"),
    Sample(SampleType.EXECUTABLE, "SayHello.c", "StringObfuscator"),
    Sample(SampleType.LIBRARY, "SayHelloLibrary.c", "StringObfuscator"),
    Sample(
        SampleType.EXECUTABLE,
        "NumberClassifier.c",
        "Everything",
    ),
]


def build_sample(sample_with_context: Tuple[Sample, Context]):
    (sample, context) = sample_with_context

    for opt_type in ["O0", "O2"]:
        print(f"Building sample {sample.output_name(opt_type, False)}...")
        sample.build(context, opt_type, False)

        print(f"Building sample {sample.output_name(opt_type)}...")
        sample.build(context, opt_type)


if __name__ == "__main__":
    # NOTE: I hate Python's argument parser (irrationally so, due to it not
    # using capitalization anywhere) but I care more about this project not
    # having any Pip dependencies than I do fixing it.
    parser = ArgumentParser()
    parser.add_argument(
        "-c",
        dest="clang",
        type=str,
        help="path to Clang",
        metavar="CLANG",
        required=True,
    )
    parser.add_argument(
        "-p",
        dest="plugin",
        type=str,
        help="path to Limoncello plugin",
        metavar="PLUGIN",
        required=True,
    )
    parser.add_argument(
        "-f", dest="filter", type=str, help="filter samples to build", metavar="FILTER", default="*"
    )
    parser.add_argument(
        "-j",
        dest="jobs",
        type=int,
        help="number of threads to use",
        metavar="N",
        default=8,
    )

    args = parser.parse_args()

    context = Context(clang_path=args.clang, plugin_path=args.plugin)
    samples = [(s, context) for s in ALL_SAMPLES if fnmatch(s.name(), args.filter)]

    build_pool = multiprocessing.Pool(processes=args.jobs)
    build_pool.map(build_sample, samples)  # pyright: ignore
