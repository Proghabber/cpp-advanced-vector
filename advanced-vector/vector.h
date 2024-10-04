#include <stdexcept>
#include <utility>


// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
public:
    Optional() = default;

    Optional(const T& value){
        info_ = new (&data_[0]) T(value);
        is_initialized_ = true;
    }

    Optional(T&& value){
        info_ = new (&data_[0]) T(std::move(value));
        is_initialized_ = true;
    }

    Optional(const Optional& other){
        if (!other.HasValue()){
            return;
        }
        info_ = new (&data_[0]) T(other.Value());
        is_initialized_ = true;
    }

    Optional(Optional&& other){
        if (!other.HasValue()){
            return;
        }
        info_ = new (&data_[0]) T(std::move(other).Value());
        is_initialized_ = true;
    }

    Optional& operator=(const T& value){
        if (HasValue()){
            *info_ = value;
            return *this;
        }
        info_ = new (&data_[0]) T(value); 
        is_initialized_ = true;
        return *this;
    }

    Optional& operator=(T&& rhs){
         if (HasValue()){
            *info_ = std::move(rhs);
            return *this;
        }
        info_ = new (&data_[0]) T(std::move(rhs)); 
        is_initialized_ = true;
        return *this;
    }

    Optional& operator=(const Optional& rhs){
        if(this != &rhs){
            if (HasValue() && !rhs.HasValue()){
                Reset();
            } else if (HasValue() && rhs.HasValue()){   
                *info_ = rhs.Value();
            } else if (!HasValue() && rhs.HasValue()){
                info_ = new (&data_[0]) T(rhs.Value());
                is_initialized_ = true;
            }
        }
        return *this;  
    }

   Optional& operator=(Optional&& rhs){
        if (HasValue() && !rhs.HasValue()){
            Reset();
        } else if (HasValue() && rhs.HasValue()){   
            *info_ = std::move(rhs).Value();
        } else if (!HasValue() && rhs.HasValue()){
            info_ = new (&data_[0]) T(std::move(rhs).Value());
            is_initialized_ = true;
        }  
        return *this;
   }

    ~Optional(){  
        if (is_initialized_){
            info_->~T();
        }
        info_ = nullptr;
    }

    bool HasValue() const{
        return is_initialized_;
    }

    T&& operator*()&& {
        return std::move(*info_);
    }

    T& operator*()& {
        return *info_;
    }

    const T& operator*() const& {
        return *info_;
    }
    // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
    // Эти проверки остаются на совести программиста

    T* operator->(){
        return info_;
    }
    const T* operator->() const{
        return info_;
    }
    // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст

    T&& Value()&& {
        if (!is_initialized_){
            throw  BadOptionalAccess();
        } 
        return std::move(*info_);
    };

    T& Value()& {
        if (!is_initialized_){
            throw  BadOptionalAccess();
        } 
        return *info_;
    }

    const T& Value() const& {
        if (!is_initialized_){
            throw  BadOptionalAccess();
        } 
        return *info_;
    }

    void Reset(){
        if(is_initialized_){
            info_->~T();
            is_initialized_ = false;
        } else {
            return;
        }
    }

    template <typename... Args>
    T& Emplace(Args&&... args){
        if (info_ != nullptr){
            Reset();
        }
        info_ = new (&data_[0]) T((std::forward<Args>(args))...);
        is_initialized_ = true;
        return *info_;
    }

private:
    // alignas нужен для правильного выравнивания блока памяти
    alignas(T) char data_[sizeof(T)];
    T* info_ = nullptr;
    bool is_initialized_ = false;
};