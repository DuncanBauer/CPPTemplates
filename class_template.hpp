#ifndef TEMPLATECLASS_H
#define TEMPLATECLASS_H

class TemplateClass
{
    private:
        /*****************
         * Typedefs
         ****************/

    public:
        /*****************
         * Constructors
         ****************/
        // Default constructor
        TemplateClass() = default;

        // Parameterized constructors
		TemplateClass(int _exampleInt) : exampleInt(_exampleInt) {}
        
        // Copy Constructor
        // Don't deep copy textures / models
        TemplateClass(const TemplateClass& other){}

        // Move Constructor
        TemplateClass(TemplateClass&& other){}

        // Destructor
        ~TemplateClass() { texture = nullptr; }

        /*****************
         * Overloaded Operators
         ****************/
        // Assignment operator
        virtual TemplateClass& operator=(const TemplateClass& other)
        {
        }

        /*****************
         * Getters and Setters
         ****************/
		int getExampleInt() const { return this->exampleInt; }
		void setExampleInt() { this->exampleInt = _exampleInt; }

    private:
        /*****************
         * Variable Members
         ****************/
		int exampleInt;
};

#endif



private:
	/*****************
	 * Typedefs
	 ****************/

public:
	/*****************
	 * Constructors
	 ****************/
	// Default constructor
	TemplateClass() = default;

	// Parameterized constructors
	
	// Copy Constructor
	// Don't deep copy textures / models
	TemplateClass(const TemplateClass& other){}

	// Move Constructor
	TemplateClass(TemplateClass&& other){}

	// Destructor
	~TemplateClass() {}

	/*****************
	 * Overloaded Operators
	 ****************/
	 
	/*****************
	 * Getters and Setters
	 ****************/

private:
	/*****************
	 * Variable Members
	 ****************/