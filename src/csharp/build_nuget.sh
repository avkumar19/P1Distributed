#!/bin/bash
# Copyright 2020 The gRPC Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -ex

cd "$(dirname "$0")"

mkdir -p ../../artifacts

# Collect the artifacts built by the previous build step
mkdir -p nativelibs
cp -r "${EXTERNAL_GIT_ROOT}"/input_artifacts/csharp_ext_* nativelibs || true

# Collect protoc artifacts built by the previous build step
mkdir -p protoc_plugins
cp -r "${EXTERNAL_GIT_ROOT}"/input_artifacts/protoc_* protoc_plugins || true

# Add current timestamp to dev nugets
./expand_dev_version.sh

dotnet restore Grpc.sln

# To be able to build the Grpc.Core project, we also need to put grpc_csharp_ext to where Grpc.Core.csproj
# expects it.
mkdir -p ../../cmake/build
cp nativelibs/csharp_ext_linux_x64/libgrpc_csharp_ext.so ../../cmake/build

dotnet pack --configuration Release Grpc.Core.Api --output ../../artifacts
dotnet pack --configuration Release Grpc.Core --output ../../artifacts
dotnet pack --configuration Release Grpc.Core.Testing --output ../../artifacts
dotnet pack --configuration Release Grpc.Auth --output ../../artifacts
dotnet pack --configuration Release Grpc.HealthCheck --output ../../artifacts
dotnet pack --configuration Release Grpc.Reflection --output ../../artifacts
dotnet pack --configuration Release Grpc.Tools --output ../../artifacts
# rem build auxiliary packages
dotnet pack --configuration Release Grpc --output ../../artifacts
dotnet pack --configuration Release Grpc.Core.NativeDebug --output ../../artifacts
dotnet pack --configuration Release Grpc.Core.Xamarin --output ../../artifacts

# Create a zipfile with all the nugets we just created
cd ../../artifacts
zip csharp_nugets_windows_dotnetcli.zip *.nupkg
