#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>


template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        :buffer_(nullptr)
        ,capacity_(0)
    {    
        Swap(other);
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept{
        if(this != &rhs){  
            Swap(rhs);
        }
        return *this;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
}; 

template <typename Type>
class Vector {
public:
    using iterator = Type*;
    using const_iterator = const Type*;
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& rhs){
        Swap(rhs);
    }

    Vector& operator=(const Vector& rhs){
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                FillFromOther(rhs);
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept{
        if(this != &rhs){
            Swap(rhs);
        }
        return *this;
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const Type& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<Type> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        data_.Swap(new_data);
        std::destroy_n(new_data.GetAddress(), size_); 
    }

    void Swap(Vector& other) noexcept{
        this->data_.Swap(other.data_);
        std::swap(this->size_, other.size_);
    }

    void Resize(size_t new_size){
        if (size_ > new_size){
            size_t step = new_size;
            std::destroy_n((data_ + step ),  size_ - new_size);
        } else if (new_size > size_){
            Reserve(new_size);
            std::uninitialized_value_construct_n((data_ + size_), new_size - size_);
        }
        size_ = new_size;
    }

    void PushBack(Type&& value) {
        EmplaceBack(std::move(value));
    }

    void PushBack(const Type& value){
        EmplaceBack(value);
    }

    template <typename... Args>
    Type& EmplaceBack(Args&&... args){
        return *Emplace(cend(), std::forward<Args>(args)...);
    }

    void PopBack(){
        if (!size_){
            return;
        }
        Destroy(data_ + size_ - 1);
        size_--;
    }

    iterator begin() noexcept{
        return iterator{data_.GetAddress()};
    }

    iterator end() noexcept{
        return iterator{std::next(data_.GetAddress(), size_)};
    }

    const_iterator begin() const noexcept{
        return const_iterator{data_.GetAddress()};
    }

    const_iterator end() const noexcept{
        return const_iterator{std::next(data_.GetAddress(), size_)};
    }

    const_iterator cbegin() const noexcept{
        return begin();
    }

    const_iterator cend() const noexcept{
        return end();
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){
        assert(pos >= begin() && pos <= end());
        size_t index = pos - data_.GetAddress();
        iterator new_pos = nullptr;
        if (size_ == Capacity()){
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<Type> new_data(new_capacity);
            if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
                new_pos = new(new_data + index) Type(std::forward<Args>(args)...);
                std::uninitialized_move_n(begin(), index, new_data.GetAddress());
                std::uninitialized_move_n(std::next(begin(), index), size_-index, std::next(new_data.GetAddress(), index + 1));
            } else {
                new_pos = new(new_data + index) Type(std::forward<Args>(args)...);
                std::uninitialized_copy_n(begin(), index, new_data.GetAddress());
                std::uninitialized_copy_n(std::next(begin(), index), size_-index, std::next(new_data.GetAddress(), index + 1));
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);

        } else if (size_ != 0 && pos!= end()){
            if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
                Type new_elem(std::forward<Args>(args)...);
                new (data_ + size_) Type(std::move(data_[size_ - 1]));
                std::move_backward(std::next(begin(), index), std::prev(end(), 1), end());     
                data_ [index] = std::move(new_elem);  
                new_pos = begin() + index;     
            } else {
                Type new_elem(std::forward<Args>(args)...);
                new (data_ + size_) Type(data_[size_ - 1]);
                std::copy_backward(std::next(begin(), index), std::prev(end(), 1), end());
                data_ [index] = new_elem; 
                new_pos = begin() + index;   
            }        
        } else {
            new_pos = new (data_ + size_) Type(std::forward<Args>(args)...);
        }
        size_++;
        return new_pos;
    }

    iterator Erase(const_iterator pos){ /*noexcept(std::is_nothrow_move_assignable_v<Type>)*/
        assert(pos >= begin() && pos <= end()-1);
        size_t index = pos - begin();
        if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
            std::move(std::next(begin(), index + 1), end(), std::next(begin(), index));
            Destroy(std::prev(end(), 1));
           
        } else {
            std::copy(std::next(begin(), index + 1), end(), std::next(begin(), index));
            Destroy(std::prev(end(), 1));
        }
        size_--;
        return (begin() + index);
    }

    iterator Insert(const_iterator pos, const Type& value){
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, Type&& value){
        return Emplace(pos, std::move(value));
    }

private:
    void FillFromOther(const Vector& other){
        size_t index = std::min(other.size_, size_);
        auto pos = std::copy_n(other.begin(), index, begin());
        std::destroy_n(pos, size_ - index);
        std::uninitialized_copy_n(std::next(other.data_ + index), other.size_ - index, pos);
        size_ = other.size_;  
    }

    static Type* Allocate(size_t n) {
        return n != 0 ? static_cast<Type*>(operator new(n * sizeof(Type))) : nullptr;
    }

    static void Deallocate(Type* buf) noexcept {
        operator delete(buf);
    }

    static void CopyConstruct(Type* buf, const Type& elem) {
        new (buf) Type(elem);
    }

    static void Destroy(Type* buf) noexcept {
        buf->~Type();
    }

    RawMemory<Type> data_;
    size_t size_ = 0;
};