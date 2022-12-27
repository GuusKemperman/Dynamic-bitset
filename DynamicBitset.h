#pragma once
#include <cassert>
#include <vector>

/*
MIT License

Copyright(c)[2022][Guus Kemperman]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

namespace DB
{
	using byte_index = size_t;
	using bit_index = unsigned char;
	using bit = bool;

	constexpr bit_index sNumOfBitsInByte = 8;

	class byte;
	class dynamic_bitset;

	// Changing the value of a bitref also updates the value in the byte that it's from.
	// The value will always match the one from the byte that it's originally from.
	// Will lead to undefined behaviour if the byte gets destroyed or moved (e.g. when 
	// the underlying vector of a dynamic_bitset resizes.).
	class bit_ref
	{
	public:
		bit_ref(byte& owner, bit_index indexAtOwner) :
			mOwner(owner),
			mIndexAtOwner(indexAtOwner)
		{}

		inline operator bit() const;
		inline void operator=(bit value);

	private:
		byte& mOwner;
		bit_index mIndexAtOwner{};
	};

	class byte
	{
	public:
		byte() = default;
		byte(unsigned char data) : mData(data) {}

		inline void set(const bit_index index, bit bit)
		{
#if _CONTAINER_DEBUG_LEVEL > 0
			assert(index < sNumOfBitsInByte);
#endif // _CONTAINER_DEBUG_LEVEL > 0

			unsigned char shiftAmount = sNumOfBitsInByte - index - 1;
			mData = (mData & ~(1 << shiftAmount)) + (bit << shiftAmount);
		}

		inline bit get(const bit_index index) const
		{
#if _CONTAINER_DEBUG_LEVEL > 0
			assert(index < sNumOfBitsInByte);
#endif // _CONTAINER_DEBUG_LEVEL > 0

			const unsigned char shiftAmount = sNumOfBitsInByte - index - 1;
			return (mData & (1 << shiftAmount)) >> shiftAmount;
		}

		inline bit_ref getBitRef(const bit_index index)
		{
			return { *this, index };
		}

		inline operator unsigned char& () { return mData; }
		inline operator const unsigned char() const { return mData; }

	private:
		unsigned char mData{};
	};

	static_assert(sizeof(byte) == 1);

	inline bit_ref::operator bit() const
	{
		return mOwner.get(mIndexAtOwner);
	}

	inline void bit_ref::operator=(bit value)
	{
		mOwner.set(mIndexAtOwner, value);
	}

	// The bits here are stored as part of chars, which in turn are stored inside a vector. This
	// ensures that 1 bit is actually taking up the space of 1 bit, as opposed to std::bitset.
	// This also meanst that getting/retrieving values is going to be slower than std::bitset. If 
	// you know at compile time what size the bitset is going to be, it is highly recommended to 
	// use std::bitset. If you don't need to store/retrieve triviably copyable types in binary 
	// format, it is highly recommned to use std::vector<bool>.
	class dynamic_bitset
	{
		template<typename DerivedType>
		class IteratorBase
		{
		public:
			IteratorBase() = default;
			IteratorBase(byte_index byteIndex, bit_index bitIndex) : mByteIndex(byteIndex), mBitIndex(bitIndex) {}

			using value_type = bit;
			using difference_type = std::ptrdiff_t;
			using iterator_category = std::forward_iterator_tag;

			// Prefix increment
			DerivedType& operator++()
			{
				if (++mBitIndex == 8)
				{
					mBitIndex = 0;
					++mByteIndex;
				}
				return *static_cast<DerivedType*>(this);
			}

			// Postfix increment
			DerivedType operator++(int)
			{
				DerivedType tmp = *static_cast<DerivedType*>(this);
				++(*this);
				return tmp;
			}

			friend bool operator== (const IteratorBase& a, const IteratorBase& b)
			{
				return a.mByteIndex == b.mByteIndex
					&& a.mBitIndex == b.mBitIndex;
			};
			friend bool operator!= (const IteratorBase& a, const IteratorBase& b)
			{
				return a.mByteIndex != b.mByteIndex
					|| a.mBitIndex != b.mBitIndex;
			};

			inline bool operator<(const IteratorBase& other)
			{
				return mByteIndex < other.mByteIndex
					|| (mByteIndex == other.mByteIndex && mBitIndex < other.mBitIndex);
			}
		protected:
			byte_index mByteIndex{};
			bit_index mBitIndex{};
		};

	public:
		class iterator :
			public IteratorBase<iterator>
		{
		public:
			iterator() = default;
			iterator(dynamic_bitset* source, byte_index byteIndex, bit_index bitIndex) : IteratorBase(byteIndex, bitIndex), mSource(source) {}

			using pointer = bit_ref;
			using reference = bit_ref;

			reference operator*() const
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->getBitRef(mByteIndex, mBitIndex);
			}
			pointer operator->() const
			{
				return *(*this);
			}

			// Prefer this over getting a const reference for performance reasons.
			inline operator bit() const
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->get(mByteIndex, mBitIndex);
			}

		private:
			friend dynamic_bitset;
			dynamic_bitset* mSource{};
		};

		class const_iterator :
			public IteratorBase<const_iterator>
		{
		public:
			const_iterator() = default;
			const_iterator(const dynamic_bitset* source, byte_index byteIndex, bit_index bitIndex) : IteratorBase(byteIndex, bitIndex), mSource(source) {}

			using pointer = bit;
			using reference = bit; 

			reference operator*() const
			{ 
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->get(mByteIndex, mBitIndex);
			}
			pointer operator->() const 
			{ 
				return *(*this);
			}

			inline operator bit() const
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->get(mByteIndex, mBitIndex);
			}

		private:
			friend dynamic_bitset;
			const dynamic_bitset* mSource{};
		};

		inline iterator begin()
		{
			return begin<iterator>(this);
		}

		inline iterator end()
		{
			return end<iterator>(this);
		};

		inline const_iterator begin() const
		{
			return begin<const_iterator>(this);
		}

		inline const_iterator end() const
		{
			return end<const_iterator>(this);
		}

		inline bit get(byte_index byteIndex, bit_index bitIndex) const
		{
#if _ITERATOR_DEBUG_LEVEL > 0
			assert(byteIndex < mData.size()
				|| (byteIndex == mData.size() && bitIndex < mIncompleteByte.mNumOfBits));
#endif // _ITERATOR_DEBUG_LEVEL

			const byte& byte = byteIndex < mData.size() ? mData[byteIndex] : mIncompleteByte.mByte;
			return byte.get(bitIndex);
		}

		// Returns the bit the iterator is pointing too and increments the iterator
		inline bit get(iterator& it) const
		{
			bit returnBit = it;
			++it;
			return returnBit;
		}

		inline bit_ref getBitRef(byte_index byteIndex, bit_index bitIndex)
		{
#if _ITERATOR_DEBUG_LEVEL > 0
			assert(byteIndex < mData.size()
				|| (byteIndex == mData.size() && bitIndex < mIncompleteByte.mNumOfBits));
#endif // _ITERATOR_DEBUG_LEVEL

			byte& returnByte = byteIndex < mData.size() ? mData[byteIndex] : mIncompleteByte.mByte;
			return returnByte.getBitRef(bitIndex);
		}

		// Returns the bitref the iterator is pointing too and increments the iterator
		inline bit_ref getBitRef(iterator& it)
		{
			bit_ref returnBitref = *it;
			++it;
			return returnBitref;
		}

		template<typename TriviablyCopyableType>
		inline void push_back(const TriviablyCopyableType& value)
		{
			static_assert(std::is_trivially_copyable<TriviablyCopyableType>::value);

			const byte* data = reinterpret_cast<const byte*>(&value);
			const size_t size = sizeof(value);

			for (size_t i = 0; i < size; i++)
			{
				push_back(data[i]);
			}
		}

		inline void push_back(byte byte)
		{
			for (bit_index i = 0; i < sNumOfBitsInByte; i++)
			{
				push_back(byte.get(i));
			}
		}

		inline void push_back(bit bit)
		{
			mIncompleteByte.mByte.set(mIncompleteByte.mNumOfBits, bit);
			mIncompleteByte.mNumOfBits++;

			if (mIncompleteByte.isFull())
			{
				mData.push_back(mIncompleteByte.mByte);
				mIncompleteByte.mNumOfBits = 0;
			}
		}

		// Removes the last bit.
		inline void pop_back()
		{
			if (isThereAnIncompleteByte())
			{
				mIncompleteByte.mNumOfBits--;
				return;
			}

			assert(!mData.empty());

			mIncompleteByte.mByte = mData.back();
			mIncompleteByte.mNumOfBits = sNumOfBitsInByte - 1;

			mData.pop_back();
		}

		inline void clear()
		{
			mData.clear();
			mIncompleteByte.mNumOfBits = 0;
		}

		// Creates and returns an instance of the type by using the next sizeof(type) bytes.
		template <typename TriviablyCopyableType>
		inline TriviablyCopyableType extract(byte_index byteIndex, bit_index bitIndex)
		{
			iterator it = { this, byteIndex, bitIndex };
			return extract<TriviablyCopyableType>(it);
		}

		// Creates and returns an instance of the type by using the next sizeof(type) bytes. Increments the iterator by the size of the type.
		template <typename TriviablyCopyableType>
		static inline TriviablyCopyableType extract(iterator& it)
		{
			static_assert(std::is_trivially_copyable<TriviablyCopyableType>::value);

			constexpr size_t numOfBytes = sizeof(TriviablyCopyableType);
			TriviablyCopyableType returnValue{};
			byte* asByte = reinterpret_cast<byte*>(&returnValue);
			extract(reinterpret_cast<char*>(&returnValue), numOfBytes, it);

			return returnValue;
		}

		// Fills the destination with the bytes specified using the byteIndex and bitIndex
		inline void extract(char* destination, size_t amountOfBytesToExtract, byte_index byteIndex, bit_index bitIndex)
		{
			iterator it = { this, byteIndex, bitIndex };
			extract(destination, amountOfBytesToExtract, it);
		}

		// Fills the destination with the bytes the iterator is pointing too and increments the iterator
		static inline void extract(char* destination, size_t amountOfBytesToExtract, iterator& it)
		{
			for (size_t i = 0; i < amountOfBytesToExtract; i++)
			{
				destination[i] = getByte(it);
			}
		}

		bool isThereAnIncompleteByte() const 
		{ 
			return mIncompleteByte.mNumOfBits > 0;
		}

	private:
		// Returns the byte the iterator is pointing too and increments the iterator
		static byte getByte(iterator& iterator)
		{
			// This is much faster, it's worth it for us to check to see if this is a possibility.
			if (iterator.mBitIndex == 0)
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(iterator.mByteIndex < iterator.mSource->mData.size());
#endif // _ITERATOR_DEBUG_LEVEL
				return iterator.mSource->mData[iterator.mByteIndex];
			}

			byte returnByte{};
			for (bit_index i = 0; i < sNumOfBitsInByte; i++, ++iterator)
			{
				bit bit = iterator.mSource->get(iterator.mByteIndex, iterator.mBitIndex);
				returnByte.set(i, bit);
			}

			return returnByte;
		}

		template<typename IteratorType, typename From>
		static IteratorType begin(From* fromBitset)
		{
			return IteratorType{ fromBitset, 0, 0 };
		}

		template<typename IteratorType, typename From>
		static IteratorType end(From* fromBitset)
		{
			byte_index byteIndex = fromBitset->mData.size();
			bit_index bitIndex = 0;

			if (fromBitset->isThereAnIncompleteByte())
			{
				bitIndex = fromBitset->mIncompleteByte.mNumOfBits;
			}

			return IteratorType{ fromBitset, byteIndex, bitIndex };
		}

		std::vector<byte> mData{};

		struct IncompleteByte
		{
			byte mByte{};
			bit_index mNumOfBits = 0;

			bool isFull() const { return mNumOfBits == sNumOfBitsInByte; }
		};
		IncompleteByte mIncompleteByte{};

		bit_index mBitIndex = sNumOfBitsInByte - 1;
	};
}