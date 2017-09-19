#include "ModularNN.h"

#include <cmath>
#include <sstream>
#include <random>
#include <chrono>

#ifdef WIN32
std::mt19937_64 mt(std::chrono::system_clock::now().time_since_epoch().count());
#else
std::mt19937_64 mt(std::chrono::steady_clock::now().time_since_epoch().count());
#endif

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
	weights(new float[inputSize + 1][outputSize]) {

	for (int i = 0; i <= inputSize; ++i)
		for (int j = 0; j < outputSize; ++j)
			is >> weights[i][j];
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
	mulWeightsVec(weights, in, output.data(), inputSize, outputSize);
	applyActivationFunction<actf>(output);
}

void ModularNN::mulWeightsVec(float const **weights, float const *vec, float *dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i <= inSize; ++i) // + 1 for bias
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += weights[i][o] * vec[i];
}

ModularNN::ModularNN(std::istream &is) {
	std::string line;
	while (getline(is, line)) {
		auto layer = readLayer(std::stringstream(line));
		if (layer)
			layers.push_back(layer);
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
