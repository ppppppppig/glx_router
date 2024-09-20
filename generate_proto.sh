
echo "proto generate dir is: $1"
proto_path="./proto_generate/proto"
echo "proto dir is: ${proto_path}"

for file in "${proto_path}"/*.proto; do
    echo $file
    protoc -I=./proto_generate/proto --grpc_out=./proto_generate --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $file
    protoc -I=./proto_generate/proto --cpp_out=./proto_generate $file
done