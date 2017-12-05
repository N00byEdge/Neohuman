#include "ModularNN.h"

#include <cmath>
#include <sstream>
#include <random>
#include <chrono>
#include "ModularNN.h"

#ifdef WIN32
std::mt19937_64 nnMt(std::chrono::system_clock::now().time_since_epoch().count());
#else
std::mt19937_64 nnMt(std::chrono::steady_clock::now().time_since_epoch().count());
#endif

template <typename T>
T read(std::istream &is) {
	T temp;
	is >> temp;
	return temp;
}

template<ModularNN::ActivationFunction actFunc>
ModularNN::StandardLayer<actFunc>::StandardLayer(unsigned inputSize, unsigned outputSize) :
	inputSize(inputSize),
	outputSize(outputSize),
	weights((inputSize + 1) * outputSize) {
	genWeights();
}

template<ModularNN::ActivationFunction actFunc>
inline ModularNN::StandardLayer<actFunc>::StandardLayer(std::istream &is) :
	inputSize(read<unsigned>(is)),
	outputSize(read<unsigned>(is)),
	weights((inputSize + 1) * outputSize) {

	for (unsigned i = 0; i <= inputSize; ++i)
		for (unsigned j = 0; j < outputSize; ++j)
			is >> weights[i * outputSize + j];
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::applyActivationFunction(fv &v) {
	return applyActivationFunction<actFunc>(v.data(), v.size());
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::ReLU>(float * const data, const unsigned size) {
	for (unsigned i = 0; i < size; ++i)
		if (data[i] < 0.0f)
			data[i] *= 0.01f;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(float * const data, const unsigned size) {
	for (unsigned i = 0; i < size; ++i)
		data[i] = 1.0f / (exp(-data[i]) + 1.0f);
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Tanh>(float * const data, const unsigned size) {
	for (unsigned i = 0; i < size; ++i)
		data[i] = (2.0f / (1 + exp(-2.0f*data[i]))) - 1.0f;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Softmax>(float * const data, const unsigned size) {
	float sum = 0.0f;
	for (unsigned i = 0; i < size; ++i)
		sum += data[i];
	for (unsigned i = 0; i < size; ++i)
		data[i] /= sum;
}

template <ModularNN::ActivationFunction actf>
fv ModularNN::StandardLayer<actf>::run(fv &in) {
	fv output(outputSize);

	mulWeightsVec(weights.data(), in.data(), output.data(), inputSize, outputSize);
	applyActivationFunction<actf>(output);

	return output;
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>::write(std::ostream &os) {
	os << "ReLU";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::Sigmoid>::write(std::ostream &os) {
	os << "Sigmoid";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::Softmax>::write(std::ostream &os) {
	os << "Softmax";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::Tanh>::write(std::ostream &os) {
	os << "Tanh";
	writeWeights(os);
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::StandardLayer<actFunc>::genWeights() {
	ModularNN::initWeightMatrix<actFunc>(weights.data(), inputSize + 1, outputSize);
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::StandardLayer<actFunc>::writeWeights(std::ostream &os) {
	os << " " << inputSize << " " << outputSize;
	for (auto i = 0u; i < inputSize + 1; ++i)
		for (auto o = 0u; o < outputSize; ++o)
			os << " " << weights[i * outputSize + o];
	os << "\n";
}

void ModularNN::mulMatrixVec(float const * const mat, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += mat[i * outSize + o] * vec[i];
}

void ModularNN::mulWeightsVec(float const *weights, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += weights[i * outSize + o] * vec[i];

	for (unsigned o = 0; o < outSize; ++o) // bias
		dest[o] += weights[inSize * outSize + o];
}

void ModularNN::addVectors(float *const target, float const *const source, const unsigned count) {
	for (unsigned i = 0; i < count; ++i)
		target[i] += source[i];
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::initWeightMatrix(float *const weights, unsigned inputSize, unsigned outputSize) {
	for (auto i = 0u; i < inputSize; ++i)
		initWeightVector<actFunc>(weights + outputSize * i, inputSize, outputSize);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::ReLU>(float *const weights, unsigned inputSize, unsigned outputSize) {
	float r = sqrt(2.0f / (inputSize + outputSize));
	std::uniform_real_distribution<float> dist(-r, r);
	for (auto o = 0u; o < outputSize; ++o)
		weights[o] = dist(nnMt);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::Sigmoid>(float *const weights, unsigned inputSize, unsigned outputSize) {
	float r = sqrt(6.0f / (inputSize + outputSize)) / 4.0f;
	std::uniform_real_distribution<float> dist(-r, r);
	for (auto o = 0u; o < outputSize; ++o)
		weights[o] = dist(nnMt);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::Softmax>(float *const weights, unsigned inputSize, unsigned outputSize) {
	return initWeightVector<ModularNN::ActivationFunction::Sigmoid>(weights, inputSize, outputSize);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::Tanh>(float *const weights, unsigned inputSize, unsigned outputSize) {
	float r = sqrt(6.0f / (inputSize + outputSize));
	std::uniform_real_distribution<float> dist(-r, r);
	for (auto o = 0u; o < outputSize; ++o)
		weights[o] = dist(nnMt);
}

ModularNN::ModularNN(std::vector<std::unique_ptr<Layer>> &layers) {
	for (auto &l : layers)
		this->layers.push_back(std::move(l));
}

ModularNN::ModularNN(ModularNN &&stealLayersFrom) {
	for (auto &l : stealLayersFrom.layers)
		layers.push_back(std::move(l));

	stealLayersFrom.layers.clear();
}

ModularNN::ModularNN(std::istream &is) {
	std::string line;
	while (getline(is, line)) {
		auto layer = std::move(readLayer(std::stringstream(line)));
		if (layer)
			layers.push_back(std::move(layer));
	}
}

fv ModularNN::run(fv input) {
	for (auto &l : layers)
		input = l->run(input);
	return input;
}

void ModularNN::write(std::ostream &os) {
	for (auto &l : layers)
		l->write(os);
	os << std::flush;
}

ModularNN ModularNN::genDivNN() {
	std::stringstream in, out;
	write(in);

	std::normal_distribution<double> dist(0.0f, I);
	std::string line;
	while (getline(in, line)) {
		std::string layerType = "";
		std::stringstream sline(line);
		sline >> layerType;
		out << layerType;
		if (layerType == "LSTM") {
			int inSize, outSize, mSize;
			sline >> inSize >> outSize >> mSize;
			out << " " << inSize << " " << outSize << " " << mSize;
		}
		else {
			int inSize, outSize;
			sline >> inSize >> outSize;
			out << " " << inSize << " " << outSize;
		}
		float temp;
		while (sline >> temp)
			out << " " << (float)dist(nnMt) + temp;
		out << std::endl;
	}

	return ModularNN(out);
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
	if (layerType == "LSTM")
		return std::make_unique<ModularNN::LSTM>(is);

	return nullptr;
}

ModularNN::LSTM::LSTM(unsigned inputSize, unsigned outputSize, unsigned mSize) : inputSize(inputSize), outputSize(outputSize), mSize(mSize) {
	resizeWeights();

	initWeightMatrix<ActivationFunction::ReLU>(Wmx.data(), inputSize, mSize);
	initWeightMatrix<ActivationFunction::ReLU>(Wmh.data(), outputSize, mSize);
	initWeightVector<ActivationFunction::ReLU>(bm.data(), (inputSize + outputSize)/2, mSize);

	initWeightMatrix<ActivationFunction::ReLU>(Whx.data(), inputSize, outputSize);
	initWeightMatrix<ActivationFunction::ReLU>(Whm.data(), mSize, outputSize);
	initWeightVector<ActivationFunction::ReLU>(bhr.data(), (inputSize + mSize)/2, outputSize);

	initWeightMatrix<ActivationFunction::Sigmoid>(Wix.data(), inputSize, outputSize);
	initWeightMatrix<ActivationFunction::Sigmoid>(Wim.data(), mSize, outputSize);
	initWeightVector<ActivationFunction::Sigmoid>(bi.data(), (inputSize + mSize) / 2, outputSize);

	initWeightMatrix<ActivationFunction::Sigmoid>(Wox.data(), inputSize, outputSize);
	initWeightMatrix<ActivationFunction::Sigmoid>(Wom.data(), mSize, outputSize);
	initWeightVector<ActivationFunction::Sigmoid>(bo.data(), (inputSize + mSize) / 2, outputSize);

	initWeightMatrix<ActivationFunction::Sigmoid>(Wfx.data(), inputSize, outputSize);
	initWeightMatrix<ActivationFunction::Sigmoid>(Wfm.data(), mSize, outputSize);
	initWeightVector<ActivationFunction::Sigmoid>(bf.data(), (inputSize + mSize) / 2, outputSize);

	initWeightVector<ActivationFunction::ReLU>(bc.data(), outputSize, outputSize);
	initWeightVector<ActivationFunction::Tanh>(bh.data(), outputSize, outputSize);
}

ModularNN::LSTM::LSTM(std::istream &is) : inputSize(read<unsigned>(is)), outputSize(read<unsigned>(is)), mSize(read<unsigned>(is)) {
	resizeWeights();

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned m = 0; m < mSize; ++m)
			is >> Wmx[i * mSize + m];

	for (unsigned o = 0; o < outputSize; ++o)
		for (unsigned m = 0; m < mSize; ++m)
			is >> Wmh[o * mSize + m];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Whx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Whm[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wix[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wim[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wox[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wom[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wfx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wfm[m * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		is >> bm[m];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bhr[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bi[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bo[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bf[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bc[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bh[o];

	for (unsigned o = 0; o < outputSize; ++o)
		hState[o] = 0.0f, cState[o] = 0.0f;
}

fv ModularNN::LSTM::run(fv &input) {
	auto m = std::vector<float>(mSize);
	mulMatrixVec(Wmx.data(), input.data(), m.data(), inputSize, mSize);
	mulMatrixVec(Wmh.data(), hState.data(), m.data(), outputSize, mSize);
	addVectors(m.data(), bm.data(), mSize);
	
	auto hr = std::vector<float>(outputSize);
	mulMatrixVec(Whx.data(), input.data(), hr.data(), inputSize, outputSize);
	mulMatrixVec(Whm.data(), m.data(), hr.data(), mSize, outputSize);
	addVectors(m.data(), bm.data(), outputSize);

	auto i = std::vector<float>(outputSize);
	mulMatrixVec(Wix.data(), input.data(), i.data(), inputSize, outputSize);
	mulMatrixVec(Wim.data(), m.data(), i.data(), mSize, outputSize);
	addVectors(i.data(), bi.data(), outputSize);
	applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(i.data(), outputSize);

	auto o = std::vector<float>(outputSize);
	mulMatrixVec(Wox.data(), input.data(), o.data(), inputSize, outputSize);
	mulMatrixVec(Wom.data(), m.data(), o.data(), mSize, outputSize);
	addVectors(o.data(), bo.data(), outputSize);
	applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(o.data(), outputSize);

	auto f = std::vector<float>(outputSize);
	mulMatrixVec(Wfx.data(), input.data(), f.data(), inputSize, outputSize);
	mulMatrixVec(Wfm.data(), m.data(), f.data(), mSize, outputSize);
	addVectors(f.data(), bf.data(), outputSize);
	applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(f.data(), outputSize);

	for (unsigned on = 0; on < outputSize; ++on)
		cState[on] = f[on] * cState[on] + i[on] * hr[on] + bc[on], hState[on] = cState[on] * o[on] + bh[on];
	applyActivationFunction<ModularNN::ActivationFunction::Tanh>(hState.data(), outputSize);

	return hState;
}

void ModularNN::LSTM::write(std::ostream &os) {
	os << "LSTM " << inputSize << " " << outputSize << " " << mSize;
	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned m = 0; m < mSize; ++m)
			os << " " << Wmx[i * mSize + m];

	for (unsigned o = 0; o < outputSize; ++o)
		for (unsigned m = 0; m < mSize; ++m)
			os << " " << Wmh[o * mSize + m];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Whx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Whm[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wix[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wim[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wox[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wom[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wfx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wfm[m * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		os << " " << bm[m];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bhr[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bi[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bo[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bf[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bc[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bh[o];

	for (unsigned o = 0; o < outputSize; ++o)
		hState[o] = 0.0f, cState[o] = 0.0f;

	os << "\n";
}

void ModularNN::LSTM::resizeWeights() {
	// Set weight matrix size
	Wmx.resize(inputSize  * mSize);
	Wmh.resize(outputSize * mSize);
	Whx.resize(inputSize  * outputSize);
	Whm.resize(mSize      * outputSize);
	Wix.resize(inputSize  * outputSize);
	Wim.resize(mSize      * outputSize);
	Wox.resize(inputSize  * outputSize);
	Wom.resize(mSize      * outputSize);
	Wfx.resize(inputSize  * outputSize);
	Wfm.resize(mSize      * outputSize);

	// Set bias size
	bm.resize(mSize);
	bhr.resize(outputSize);
	bi.resize(outputSize);
	bo.resize(outputSize);
	bf.resize(outputSize);
	bc.resize(outputSize);
	bh.resize(outputSize);

	// Set state size
	hState.resize(outputSize);
	cState.resize(outputSize);
}

namespace {
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>;
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::Sigmoid>;
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::Tanh>;
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::Softmax>;
}
