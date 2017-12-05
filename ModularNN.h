#pragma once

#include <vector>
#include <iostream>
#include <memory>

using fv = std::vector <float>;

constexpr float I = 1e-1f;

struct ModularNN {
	// NN related definitions
	struct Layer {
		virtual ~Layer() = default;
		virtual fv run(fv &input) = 0;
		virtual void write(std::ostream &) = 0;
	};

	enum class ActivationFunction {
		ReLU,
		Sigmoid,
		Tanh,
		Softmax,
	};

	template <ActivationFunction actFunc>
	static void applyActivationFunction(fv &v);
	template <ActivationFunction actFunc>
	static void applyActivationFunction(float *const data, const unsigned size);

	template <ActivationFunction actFunc>
	struct StandardLayer : public Layer {
		virtual ~StandardLayer() = default;
		StandardLayer(unsigned inputSize, unsigned outputSize);
		StandardLayer(std::istream &);
		virtual fv run(fv &input) override;
		virtual void write(std::ostream &) override;

	private:
		void genWeights();
		void writeWeights(std::ostream &);
		const unsigned inputSize, outputSize;
		std::vector <float> weights;
	};

	struct LSTM : public Layer {
		LSTM(unsigned inputSize, unsigned outputSize, unsigned mSize);
		LSTM(std::istream &);
		~LSTM() = default;
		virtual fv run(fv &input) override;
		virtual void write(std::ostream &) override;

	private:
		void resizeWeights();

		// Sizes
		const unsigned inputSize, outputSize, mSize;

		// Weight matrices
		std::vector <float> Wmx, Wmh, Whx, Whm, Wix, Wim, Wox, Wom, Wfx, Wfm;
		//float *const Wmx, *const Wmh, *const Whx, *const Whm, *const Wix, *const Wim, *const Wox, *const Wom, *const Wfx, *const Wfm;

		// Biases
		std::vector <float> bm, bhr, bi, bo, bf, bc, bh;
		//float *const bm, *const bhr, *const bi, *const bo, *const bf, *const bc, *const bh;

		// State
		std::vector <float> hState, cState;
		//float *const hState, *const cState;
	};

	// ModularNN static functions
	static void mulMatrixVec(float const *const mat, float const *const vec, float *const dest, const unsigned inSize, const unsigned outSize);
	static void mulWeightsVec(float const *const weights, float const *const vec, float *const dest, const unsigned inSize, const unsigned outSize);
	static void addVectors(float *const target, float const *const source, const unsigned count);
	template <ActivationFunction>
	static void initWeightMatrix(float *const weights, unsigned inputSize, unsigned outputSize);
	template <ActivationFunction>
	static void initWeightVector(float *const weights, unsigned inputSize, unsigned outputSize);

	// ModularNN functions
	ModularNN(std::vector <std::unique_ptr<Layer>> &layers);
	ModularNN(ModularNN &&stealLayersFrom);
	ModularNN(std::istream &is);
	~ModularNN() = default;
	fv run(fv input);
	void write(std::ostream &os);
	ModularNN genDivNN();

private:
	std::unique_ptr<Layer> readLayer(std::istream &is);
	std::vector <std::unique_ptr<Layer>> layers;
};
