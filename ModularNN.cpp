#include "ModularNN.h"

#include <cmath>
#include <sstream>

template <typename T>
T read(std::istream &is) {
	T temp;
	is >> temp;
	return temp;
}

template<ModularNN::ActivationFunction actFunc>
inline ModularNN::StandardLayer<actFunc>::StandardLayer(std::istream &is) :
	inputSize(read<unsigned>(is)),
	outputSize(read<unsigned>(is)),
	weights(new float *[inputSize + 1]) {

	for (unsigned i = 0; i <= inputSize; ++i)
		weights[i] = new float[outputSize];

	for (unsigned i = 0; i <= inputSize; ++i)
		for (unsigned j = 0; j < outputSize; ++j)
			is >> weights[i][j];
}

template<ModularNN::ActivationFunction actFunc>
ModularNN::StandardLayer<actFunc>::~StandardLayer() {
	for (unsigned i = 0; i <= inputSize; ++i)
		delete[] weights[i];

	delete[] weights;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::ReLU>(fv &v) {
	for (auto &f : v)
		if (f < 0.0f)
			f *= 0.01f;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(fv &v) {
	for (auto &f : v)
		f = 1.0f / (exp(-f) + 1.0f);
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Tanh>(fv &v) {
	for (auto &f : v)
		f = (2.0f / (1 + exp(-2.0f*f))) - 1.0f;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Softmax>(fv &v) {
	float sum = 0.0f;
	for (auto &f : v)
		sum += f;
	for (auto &f : v)
		f /= sum;
}

template <ModularNN::ActivationFunction actf>
fv ModularNN::StandardLayer<actf>::run(fv &in) {
	fv output(outputSize);
	mulWeightsVec(weights, in.data(), output.data(), inputSize, outputSize);
	applyActivationFunction<actf>(output);
}

void ModularNN::mulWeightsVec(float const * const *weights, float const *vec, float *dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += weights[i][o] * vec[i];

	for (unsigned o = 0; o < outSize; ++o) // bias
		dest[o] += weights[inSize][o];
}

ModularNN::ModularNN(std::istream &is) {
	std::string line;
	while (getline(is, line)) {
		auto layer = std::move(readLayer(std::stringstream(line)));
		if (layer)
			layers.push_back(std::move(layer));
	}
}

fv ModularNN::run(fv & input) {
	for (auto &l : layers)
		input = l->run(input);
	return input;
}

std::unique_ptr<ModularNN::Layer> ModularNN::readLayer(std::istream &is) {
	std::string layerType = "";
	is >> layerType;
	if (layerType == "ReLU")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>>(is);
	if (layerType == "Sigmoid")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::Sigmoid>>(is);
	if (layerType == "Softmax")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::Softmax>>(is);
	if (layerType == "Tanh")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::Tanh>>(is);

	return nullptr;
}
